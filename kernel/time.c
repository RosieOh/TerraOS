// kernel/time.c - 시간 관리
// 타이머 기반 시간 추적

#include <stdint.h>

// 시스템 시작 후 경과 시간 (초)
static volatile uint32_t system_seconds = 0;

// 타이머 틱 카운터 (100Hz = 10ms per tick)
extern volatile uint64_t timer_ticks;

// 시간 업데이트 (타이머 인터럽트에서 호출)
void time_update(void) {
    // 100Hz 타이머이므로 100틱 = 1초
    system_seconds = (uint32_t)(timer_ticks / 100);
}

// 현재 시간 가져오기 (초)
uint32_t time_get_seconds(void) {
    time_update();
    return system_seconds;
}

// 현재 시간 가져오기 (밀리초) - 32비트로 제한
uint32_t time_get_milliseconds(void) {
    // 100Hz = 10ms per tick
    return (uint32_t)(timer_ticks * 10);
}

// 시간 포맷팅 (초를 HH:MM:SS 형식으로)
void time_format_seconds(uint32_t seconds, char* buffer, int buffer_size) {
    uint32_t hours = seconds / 3600;
    uint32_t minutes = (seconds % 3600) / 60;
    uint32_t secs = seconds % 60;
    
    // 간단한 포맷팅
    if (buffer_size >= 12) {
        buffer[0] = '0' + (hours / 10);
        buffer[1] = '0' + (hours % 10);
        buffer[2] = ':';
        buffer[3] = '0' + (minutes / 10);
        buffer[4] = '0' + (minutes % 10);
        buffer[5] = ':';
        buffer[6] = '0' + (secs / 10);
        buffer[7] = '0' + (secs % 10);
        buffer[8] = '\0';
    }
}

