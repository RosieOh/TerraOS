// kernel/pipe.c - 파이프 및 리다이렉션
// 프로세스 간 통신 및 입출력 리다이렉션

#include <stdint.h>

#define MAX_PIPES 16
#define PIPE_BUFFER_SIZE 4096

typedef struct {
    char buffer[PIPE_BUFFER_SIZE];
    uint32_t read_pos;
    uint32_t write_pos;
    uint32_t count;
    int valid;
    uint32_t reader_pid;
    uint32_t writer_pid;
} pipe_t;

static pipe_t pipes[MAX_PIPES];
static int pipe_count = 0;

// 파이프 생성
int pipe_create(uint32_t* read_fd, uint32_t* write_fd) {
    if (pipe_count >= MAX_PIPES) return -1;
    
    pipe_t* pipe = &pipes[pipe_count];
    pipe->read_pos = 0;
    pipe->write_pos = 0;
    pipe->count = 0;
    pipe->valid = 1;
    pipe->reader_pid = 0;
    pipe->writer_pid = 0;
    
    // 파일 디스크립터 할당 (간단히 인덱스 사용)
    *read_fd = 100 + pipe_count * 2;
    *write_fd = 100 + pipe_count * 2 + 1;
    
    ++pipe_count;
    return 0;
}

// 파이프 읽기
uint32_t pipe_read(int pipe_idx, char* buf, uint32_t len) {
    if (pipe_idx < 0 || pipe_idx >= MAX_PIPES || !pipes[pipe_idx].valid) {
        return (uint32_t)-1;
    }
    
    pipe_t* pipe = &pipes[pipe_idx];
    uint32_t read = 0;
    
    while (read < len && pipe->count > 0) {
        buf[read++] = pipe->buffer[pipe->read_pos];
        pipe->read_pos = (pipe->read_pos + 1) % PIPE_BUFFER_SIZE;
        --pipe->count;
    }
    
    return read;
}

// 파이프 쓰기
uint32_t pipe_write(int pipe_idx, const char* buf, uint32_t len) {
    if (pipe_idx < 0 || pipe_idx >= MAX_PIPES || !pipes[pipe_idx].valid) {
        return (uint32_t)-1;
    }
    
    pipe_t* pipe = &pipes[pipe_idx];
    uint32_t written = 0;
    
    while (written < len && pipe->count < PIPE_BUFFER_SIZE) {
        pipe->buffer[pipe->write_pos] = buf[written++];
        pipe->write_pos = (pipe->write_pos + 1) % PIPE_BUFFER_SIZE;
        ++pipe->count;
    }
    
    return written;
}

// 파이프 초기화
void pipe_init(void) {
    pipe_count = 0;
    for (int i = 0; i < MAX_PIPES; ++i) {
        pipes[i].valid = 0;
    }
}

