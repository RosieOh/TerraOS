// kernel/exec.c - 명령어 실행 시스템
// 파이프, 리다이렉션 실제 실행

#include <stdint.h>

// 외부 함수들
extern void console_write(const char* s, uint8_t color);
extern void console_putc(char c, uint8_t color);
extern uint8_t make_color(uint8_t fg, uint8_t bg);
extern int ramfs_read(const char* name, const char** out_data, uint32_t* out_size);
extern int ramfs_write(const char* name, const char* data, uint32_t size);
extern int ramfs_create(const char* name, int is_dir);
extern uint32_t pipe_create(uint32_t* read_fd, uint32_t* write_fd);
extern uint32_t devfs_write(const char* name, const void* buffer, uint32_t size);
extern uint32_t devfs_read(const char* name, void* buffer, uint32_t size);

// 문자열 비교 헬퍼
static int str_compare(const char* a, const char* b) {
    if (!a || !b) return 0;
    while (*a && *b && *a == *b) {
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

// 문자열 길이
static int str_len(const char* s) {
    if (!s) return 0;
    int n = 0;
    while (*s++) ++n;
    return n;
}

// 파이프 실행
int exec_pipe(const char* cmd1, const char* arg1, const char* cmd2, const char* arg2) {
    uint8_t color = make_color(0x0F, 0x01);
    
    // 파이프 생성
    uint32_t read_fd = 0, write_fd = 0;
    if (pipe_create(&read_fd, &write_fd) != 0) {
        console_write("Failed to create pipe\n", color);
        return -1;
    }
    
    // 첫 번째 명령어 실행 (출력을 파이프에)
    // 간단히 구현: 첫 번째 명령어의 출력을 버퍼에 저장
    char pipe_buffer[4096];
    uint32_t pipe_size = 0;
    
    // 첫 번째 명령어 실행 (간단히 echo나 cat만 지원)
    if (str_compare(cmd1, "echo")) {
        if (arg1) {
            int i = 0;
            while (arg1[i] && pipe_size < sizeof(pipe_buffer) - 1) {
                pipe_buffer[pipe_size++] = arg1[i++];
            }
        }
    } else if (str_compare(cmd1, "cat")) {
        if (arg1) {
            const char* data = 0;
            uint32_t size = 0;
            if (ramfs_read(arg1, &data, &size)) {
                for (uint32_t i = 0; i < size && pipe_size < sizeof(pipe_buffer) - 1; ++i) {
                    pipe_buffer[pipe_size++] = data[i];
                }
            }
        }
    }
    
    // 두 번째 명령어 실행 (파이프에서 입력 읽기)
    if (str_compare(cmd2, "cat")) {
        // cat은 파일을 읽지만, 파이프에서는 출력만
        console_write(pipe_buffer, color);
        console_putc('\n', color);
    } else if (str_compare(cmd2, "echo")) {
        // echo는 인자를 출력
        console_write(pipe_buffer, color);
        console_putc('\n', color);
    }
    
    return 0;
}

// 리다이렉션 실행
int exec_redirect(const char* cmd, const char* arg, const char* file, int is_output) {
    uint8_t color = make_color(0x0F, 0x01);
    
    if (is_output) {
        // 출력 리다이렉션 (>)
        if (str_compare(cmd, "echo")) {
            if (arg) {
                // 파일에 쓰기
                if (ramfs_write(file, arg, str_len(arg)) == 0) {
                    console_write("Output redirected to: ", color);
                    console_write(file, color);
                    console_putc('\n', color);
                    return 0;
                }
            }
        }
    } else {
        // 입력 리다이렉션 (<)
        const char* data = 0;
        uint32_t size = 0;
        if (ramfs_read(file, &data, &size)) {
            if (str_compare(cmd, "cat")) {
                // cat은 파일 내용 출력
                for (uint32_t i = 0; i < size; ++i) {
                    console_putc(data[i], color);
                }
                return 0;
            }
        }
    }
    
    return -1;
}

