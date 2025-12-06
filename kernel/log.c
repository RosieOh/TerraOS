// kernel/log.c - 커널 로깅 시스템
// 디버깅 및 시스템 정보 기록

#include <stdint.h>

// 로그 레벨
typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARN = 2,
    LOG_ERROR = 3
} log_level_t;

static log_level_t current_log_level = LOG_INFO;

// 로그 버퍼 (순환 버퍼)
#define LOG_BUFFER_SIZE 1024
#define LOG_ENTRY_SIZE 128

typedef struct {
    char entries[LOG_BUFFER_SIZE][LOG_ENTRY_SIZE];
    uint32_t write_pos;
    uint32_t count;
} log_buffer_t;

static log_buffer_t log_buffer;

// 콘솔 출력 함수들
extern void console_write(const char* s, uint8_t color);
extern void console_putc(char c, uint8_t color);
extern uint8_t make_color(uint8_t fg, uint8_t bg);
extern void console_write_dec(uint32_t value, uint8_t color);

// 간단한 문자열 복사
static void str_copy(char* dest, const char* src, int max_len) {
    int i = 0;
    while (src[i] && i < max_len - 1) {
        dest[i] = src[i];
        ++i;
    }
    dest[i] = '\0';
}

// 로그 메시지 기록
void klog(log_level_t level, const char* message) {
    if (level < current_log_level) return;
    
    // 로그 버퍼에 저장
    if (log_buffer.count < LOG_BUFFER_SIZE) {
        str_copy(log_buffer.entries[log_buffer.write_pos], message, LOG_ENTRY_SIZE);
        log_buffer.write_pos = (log_buffer.write_pos + 1) % LOG_BUFFER_SIZE;
        ++log_buffer.count;
    } else {
        // 버퍼가 가득 차면 덮어쓰기
        str_copy(log_buffer.entries[log_buffer.write_pos], message, LOG_ENTRY_SIZE);
        log_buffer.write_pos = (log_buffer.write_pos + 1) % LOG_BUFFER_SIZE;
    }
    
    // 콘솔에도 출력 (레벨에 따라 색상 변경)
    uint8_t color = make_color(0x0F, 0x01);  // 기본 흰색
    if (level == LOG_WARN) {
        color = make_color(0x0E, 0x01);  // 노란색
    } else if (level == LOG_ERROR) {
        color = make_color(0x0C, 0x01);  // 빨간색
    }
    
    console_write("[LOG] ", color);
    console_write(message, color);
    console_putc('\n', color);
}

// 로그 레벨 설정
void klog_set_level(log_level_t level) {
    current_log_level = level;
}

// 로그 버퍼 읽기
uint32_t klog_get_entries(char** out_entries, uint32_t max_count) {
    if (!out_entries) return 0;
    
    uint32_t count = log_buffer.count;
    if (count > max_count) count = max_count;
    
    uint32_t start_pos = (log_buffer.write_pos + LOG_BUFFER_SIZE - count) % LOG_BUFFER_SIZE;
    for (uint32_t i = 0; i < count; ++i) {
        out_entries[i] = log_buffer.entries[(start_pos + i) % LOG_BUFFER_SIZE];
    }
    
    return count;
}

// 로그 초기화
void klog_init(void) {
    log_buffer.write_pos = 0;
    log_buffer.count = 0;
    klog(LOG_INFO, "Logging system initialized");
}

