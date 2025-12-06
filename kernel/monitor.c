// kernel/monitor.c - 성능 모니터링
// top, vmstat 같은 도구

#include <stdint.h>

// 외부 함수들
extern void console_write(const char* s, uint8_t color);
extern void console_putc(char c, uint8_t color);
extern uint8_t make_color(uint8_t fg, uint8_t bg);
extern void console_write_dec(uint32_t value, uint8_t color);
extern void console_write_hex(uint32_t value, uint8_t color);
extern void heap_get_info(uint32_t* total, uint32_t* used, uint32_t* free);
extern uint32_t time_get_seconds(void);
extern void memory_get_info(uint32_t* total, uint32_t* used);

// 프로세스 정보 표시 (top)
void monitor_top(void) {
    uint8_t color = make_color(0x0F, 0x01);
    uint8_t header_color = make_color(0x0E, 0x01);
    
    console_write("=== terraOS Process Monitor ===\n", header_color);
    
    // 헤더
    console_write("PID  NAME         STATE    PRIORITY\n", header_color);
    console_write("------------------------------------\n", header_color);
    
    // 태스크 정보 (tasks.c에서)
    typedef struct {
        const char* name;
        uint32_t pid;
        int state;
        int priority;
    } task_info_t;
    
    extern void task_print_info(void);
    task_print_info();
    
    // 메모리 정보
    uint32_t total_mem = 0, used_mem = 0, free_mem = 0;
    heap_get_info(&total_mem, &used_mem, &free_mem);
    memory_get_info(&total_mem, &used_mem);
    
    console_write("\nMemory: ", color);
    console_write_dec(used_mem / 1024, color);
    console_write(" KB used, ", color);
    console_write_dec(free_mem / 1024, color);
    console_write(" KB free\n", color);
    
    // 업타임
    uint32_t uptime = time_get_seconds();
    console_write("Uptime: ", color);
    console_write_dec(uptime, color);
    console_write(" seconds\n", color);
}

// 시스템 통계 (vmstat)
void monitor_vmstat(void) {
    uint8_t color = make_color(0x0F, 0x01);
    uint8_t header_color = make_color(0x0B, 0x01);
    
    console_write("=== terraOS System Statistics ===\n", header_color);
    
    // 메모리 통계
    uint32_t total_mem = 0, used_mem = 0, free_mem = 0;
    heap_get_info(&total_mem, &used_mem, &free_mem);
    memory_get_info(&total_mem, &used_mem);
    
    console_write("Memory:\n", color);
    console_write("  Total: ", color);
    console_write_dec(total_mem / 1024, color);
    console_write(" KB\n", color);
    console_write("  Used:  ", color);
    console_write_dec(used_mem / 1024, color);
    console_write(" KB (", color);
    if (total_mem > 0) {
        uint32_t percent = (used_mem * 100) / total_mem;
        console_write_dec(percent, color);
        console_write("%)\n", color);
    }
    console_write("  Free:  ", color);
    console_write_dec(free_mem / 1024, color);
    console_write(" KB\n", color);
    
    // 프레임 통계
    extern void memory_get_info(uint32_t* total, uint32_t* used);
    uint32_t total_frames = 0, used_frames = 0;
    memory_get_info(&total_frames, &used_frames);
    
    console_write("\nFrames:\n", color);
    console_write("  Total: ", color);
    console_write_dec(total_frames / 4096, color);
    console_write("\n", color);
    console_write("  Used:  ", color);
    console_write_dec(used_frames / 4096, color);
    console_write("\n", color);
    console_write("  Free:  ", color);
    console_write_dec((total_frames - used_frames) / 4096, color);
    console_write("\n", color);
    
    // 업타임
    uint32_t uptime = time_get_seconds();
    console_write("\nUptime: ", color);
    console_write_dec(uptime, color);
    console_write(" seconds\n", color);
}

