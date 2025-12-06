// kernel/syscall.c - 시스템 콜 인터페이스
// 유저 모드 프로그램이 커널 서비스를 요청하는 메커니즘

#include <stdint.h>

// 시스템 콜 번호 정의
#define SYS_WRITE   1
#define SYS_READ    2
#define SYS_EXIT    3
#define SYS_FORK    4
#define SYS_GETPID  5
#define SYS_OPEN    6
#define SYS_CLOSE   7
#define SYS_WAIT    8
#define SYS_EXEC    9
#define SYS_CHDIR   10
#define SYS_GETCWD  11
#define SYS_STAT    12
#define SYS_PIPE    13
#define SYS_DUP2    14

// 파일 디스크립터 관리 (앞에 정의하여 sys_read/sys_write에서 사용)
#define MAX_FDS 16
#define FD_STDIN  0
#define FD_STDOUT 1
#define FD_STDERR 2

typedef struct {
    int valid;
    const char* filename;
    uint32_t offset;
    int is_pipe;        // 파이프 여부
    int pipe_idx;       // 파이프 인덱스 (pipe.c의 pipes 배열 인덱스)
    int is_read_end;    // 읽기 끝인지 쓰기 끝인지 (파이프용)
} fd_entry_t;

static fd_entry_t fd_table[MAX_FDS];
static int fd_table_initialized = 0;

static void fd_table_init(void) {
    if (fd_table_initialized) return;
    
    // 표준 입출력 초기화
    fd_table[FD_STDIN].valid = 1;
    fd_table[FD_STDIN].filename = "<stdin>";
    fd_table[FD_STDIN].offset = 0;
    fd_table[FD_STDIN].is_pipe = 0;
    
    fd_table[FD_STDOUT].valid = 1;
    fd_table[FD_STDOUT].filename = "<stdout>";
    fd_table[FD_STDOUT].offset = 0;
    fd_table[FD_STDOUT].is_pipe = 0;
    
    fd_table[FD_STDERR].valid = 1;
    fd_table[FD_STDERR].filename = "<stderr>";
    fd_table[FD_STDERR].offset = 0;
    fd_table[FD_STDERR].is_pipe = 0;
    
    fd_table_initialized = 1;
}

// 시스템 콜 핸들러 함수들
static uint32_t sys_write(uint32_t fd, const char* buf, uint32_t len) {
    fd_table_init();
    
    if (fd >= MAX_FDS || !fd_table[fd].valid) {
        return (uint32_t)-1;
    }
    
    // 파이프인 경우
    if (fd_table[fd].is_pipe && !fd_table[fd].is_read_end) {
        extern uint32_t pipe_write(int pipe_idx, const char* buf, uint32_t len);
        return pipe_write(fd_table[fd].pipe_idx, buf, len);
    }
    
    // stdout/stderr인 경우
    if (fd == FD_STDOUT || fd == FD_STDERR) {
        extern void console_write(const char* s, uint8_t color);
        extern uint8_t make_color(uint8_t fg, uint8_t bg);
        extern void console_putc(char c, uint8_t color);
        
        uint8_t color = make_color(0x0F, 0x01);  // 흰색
        for (uint32_t i = 0; i < len && buf[i] != '\0'; ++i) {
            if (buf[i] == '\n') {
                console_putc('\n', color);
            } else {
                console_putc(buf[i], color);
            }
        }
        return len;
    }
    
    return (uint32_t)-1;
}

static uint32_t sys_read(uint32_t fd, char* buf, uint32_t len) {
    fd_table_init();
    
    if (fd >= MAX_FDS || !fd_table[fd].valid) {
        return (uint32_t)-1;
    }
    
    // 파이프인 경우
    if (fd_table[fd].is_pipe && fd_table[fd].is_read_end) {
        extern uint32_t pipe_read(int pipe_idx, char* buf, uint32_t len);
        return pipe_read(fd_table[fd].pipe_idx, buf, len);
    }
    
    // stdin인 경우
    if (fd == FD_STDIN) {
        extern char keyboard_read_char(void);
        
        uint32_t i = 0;
        while (i < len - 1) {
            char c = keyboard_read_char();
            if (c == '\n') {
                buf[i++] = '\n';
                break;
            }
            buf[i++] = c;
        }
        buf[i] = '\0';
        return i;
    }
    
    return (uint32_t)-1;
}

static void sys_exit(uint32_t status) {
    (void)status;
    
    // 현재 태스크 종료 (tasks.c에서)
    extern void task_exit_current(void);
    task_exit_current();
}

static int fd_alloc(void) {
    fd_table_init();
    for (int i = 3; i < MAX_FDS; ++i) {
        if (!fd_table[i].valid) {
            fd_table[i].valid = 1;
            return i;
        }
    }
    return -1;
}

static void fd_free(int fd) {
    if (fd >= 0 && fd < MAX_FDS) {
        fd_table[fd].valid = 0;
        fd_table[fd].filename = 0;
        fd_table[fd].offset = 0;
    }
}

static uint32_t sys_open(const char* filename, uint32_t flags) {
    (void)flags;  // flags는 나중에 구현
    
    fd_table_init();
    
    // RAM 디스크에서 파일 찾기
    extern int ramfs_read(const char* name, const char** out_data, uint32_t* out_size);
    const char* data = 0;
    uint32_t size = 0;
    
    if (!ramfs_read(filename, &data, &size)) {
        return (uint32_t)-1;  // 파일을 찾을 수 없음
    }
    
    int fd = fd_alloc();
    if (fd < 0) {
        return (uint32_t)-1;  // 파일 디스크립터 할당 실패
    }
    
    fd_table[fd].filename = filename;
    fd_table[fd].offset = 0;
    
    return (uint32_t)fd;
}

static uint32_t sys_close(uint32_t fd) {
    if (fd < 3) {
        return 0;  // 표준 입출력은 닫을 수 없음
    }
    
    fd_free((int)fd);
    return 0;
}

static uint32_t sys_read_file(uint32_t fd, char* buf, uint32_t len) {
    if (fd == FD_STDIN) {
        return sys_read(fd, buf, len);
    }
    
    if (fd >= MAX_FDS || !fd_table[fd].valid) {
        return (uint32_t)-1;
    }
    
    // RAM 디스크에서 파일 읽기
    extern int ramfs_read(const char* name, const char** out_data, uint32_t* out_size);
    const char* data = 0;
    uint32_t size = 0;
    
    if (!ramfs_read(fd_table[fd].filename, &data, &size)) {
        return (uint32_t)-1;
    }
    
    uint32_t offset = fd_table[fd].offset;
    if (offset >= size) {
        return 0;  // EOF
    }
    
    uint32_t to_read = len;
    if (offset + to_read > size) {
        to_read = size - offset;
    }
    
    for (uint32_t i = 0; i < to_read; ++i) {
        buf[i] = data[offset + i];
    }
    
    fd_table[fd].offset += to_read;
    return to_read;
}

static uint32_t sys_fork(void) {
    // 현재 태스크 복제
    extern uint32_t task_fork_current_pid(void);
    uint32_t new_pid = task_fork_current_pid();
    if (new_pid == (uint32_t)-1) {
        return (uint32_t)-1;
    }
    
    // 새 프로세스의 PID 반환 (부모 프로세스)
    // 자식 프로세스는 0 반환 (나중에 개선)
    return new_pid;
}

static uint32_t sys_getpid(void) {
    // 현재 프로세스 ID 반환 (tasks.c에서)
    extern uint32_t task_get_current_pid(void);
    return task_get_current_pid();
}

static uint32_t sys_wait(uint32_t pid) {
    (void)pid;  // 간단히 구현 - 나중에 개선
    // 프로세스가 종료될 때까지 대기
    extern void task_sleep_current(void);
    task_sleep_current();
    return 0;
}

static uint32_t sys_exec(const char* filename) {
    // ELF 파일 로드 및 실행
    extern int elf_load_and_execute(const char* filename);
    int result = elf_load_and_execute(filename);
    if (result == 0) {
        // 현재 프로세스를 종료하고 새 프로세스로 전환
        extern void task_exit_current(void);
        task_exit_current();
        return 0;
    }
    return (uint32_t)-1;
}

static uint32_t sys_pipe(uint32_t* pipefd) {
    // 파이프 생성
    extern int pipe_create(uint32_t* read_fd, uint32_t* write_fd);
    uint32_t read_fd, write_fd;
    if (pipe_create(&read_fd, &write_fd) != 0) {
        return (uint32_t)-1;
    }
    
    // 파이프 인덱스 계산 (pipe_create가 100 + pipe_count * 2를 반환하므로)
    int pipe_idx = (read_fd - 100) / 2;
    
    // 파일 디스크립터 할당
    fd_table_init();
    int read_fd_idx = fd_alloc();
    int write_fd_idx = fd_alloc();
    
    if (read_fd_idx < 0 || write_fd_idx < 0) {
        if (read_fd_idx >= 0) fd_free(read_fd_idx);
        if (write_fd_idx >= 0) fd_free(write_fd_idx);
        return (uint32_t)-1;
    }
    
    // 파이프 FD 설정
    fd_table[read_fd_idx].is_pipe = 1;
    fd_table[read_fd_idx].pipe_idx = pipe_idx;  // 실제 파이프 인덱스
    fd_table[read_fd_idx].is_read_end = 1;
    fd_table[read_fd_idx].filename = "<pipe>";
    
    fd_table[write_fd_idx].is_pipe = 1;
    fd_table[write_fd_idx].pipe_idx = pipe_idx;  // 같은 파이프 인덱스
    fd_table[write_fd_idx].is_read_end = 0;
    fd_table[write_fd_idx].filename = "<pipe>";
    
    // 사용자 공간에 FD 반환
    if (pipefd) {
        pipefd[0] = read_fd_idx;
        pipefd[1] = write_fd_idx;
    }
    
    return 0;
}

static uint32_t sys_dup2(uint32_t oldfd, uint32_t newfd) {
    fd_table_init();
    
    if (oldfd >= MAX_FDS || !fd_table[oldfd].valid) {
        return (uint32_t)-1;
    }
    
    // newfd가 이미 열려있으면 닫기
    if (newfd < MAX_FDS && fd_table[newfd].valid) {
        fd_free(newfd);
    }
    
    // newfd 할당
    if (newfd >= MAX_FDS) {
        int allocated_fd = fd_alloc();
        if (allocated_fd < 0) {
            return (uint32_t)-1;
        }
        newfd = allocated_fd;
    } else {
        fd_table[newfd].valid = 1;
    }
    
    // oldfd의 내용을 newfd에 복사
    fd_table[newfd].filename = fd_table[oldfd].filename;
    fd_table[newfd].offset = fd_table[oldfd].offset;
    fd_table[newfd].is_pipe = fd_table[oldfd].is_pipe;
    fd_table[newfd].pipe_idx = fd_table[oldfd].pipe_idx;
    fd_table[newfd].is_read_end = fd_table[oldfd].is_read_end;
    
    return newfd;
}

// 시스템 콜 디스패처
uint32_t syscall_handler(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    switch (syscall_num) {
        case SYS_WRITE:
            return sys_write(arg1, (const char*)arg2, arg3);
        case SYS_READ:
            // 파일 디스크립터에 따라 분기
            if (arg1 == FD_STDIN) {
                return sys_read(arg1, (char*)arg2, arg3);
            } else {
                return sys_read_file(arg1, (char*)arg2, arg3);
            }
        case SYS_EXIT:
            sys_exit(arg1);
            return 0;
        case SYS_FORK:
            return sys_fork();
        case SYS_GETPID:
            return sys_getpid();
        case SYS_OPEN:
            return sys_open((const char*)arg1, arg2);
        case SYS_CLOSE:
            return sys_close(arg1);
        case SYS_WAIT:
            return sys_wait(arg1);
        case SYS_EXEC:
            return sys_exec((const char*)arg1);
        case SYS_CHDIR:
            {
                extern int path_chdir(const char* path);
                return path_chdir((const char*)arg1) == 0 ? 0 : (uint32_t)-1;
            }
        case SYS_GETCWD:
            {
                extern const char* path_getcwd(void);
                const char* cwd = path_getcwd();
                if (cwd && arg1) {
                    int i = 0;
                    while (cwd[i] && i < (int)arg2 - 1) {
                        ((char*)arg1)[i] = cwd[i];
                        ++i;
                    }
                    ((char*)arg1)[i] = '\0';
                    return i;
                }
                return (uint32_t)-1;
            }
        case SYS_STAT:
            {
                // 간단한 stat 구현 (파일 존재 여부만)
                extern int ramfs_read(const char* name, const char** out_data, uint32_t* out_size);
                const char* data = 0;
                uint32_t size = 0;
                if (ramfs_read((const char*)arg1, &data, &size)) {
                    // stat 구조체에 정보 채우기 (간단히)
                    if (arg2) {
                        ((uint32_t*)arg2)[0] = size;  // 파일 크기
                        ((uint32_t*)arg2)[1] = 1;      // 파일 존재
                    }
                    return 0;
                }
                return (uint32_t)-1;
            }
        case SYS_PIPE:
            return sys_pipe((uint32_t*)arg1);
        case SYS_DUP2:
            return sys_dup2(arg1, arg2);
        default:
            return (uint32_t)-1;  // 잘못된 syscall 번호
    }
}

