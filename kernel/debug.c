// kernel/debug.c - 디버깅 도구
// 스택 트레이스, 메모리 덤프 등

#include <stdint.h>

// 외부 함수들
extern void console_write(const char* s, uint8_t color);
extern void console_putc(char c, uint8_t color);
extern uint8_t make_color(uint8_t fg, uint8_t bg);
void console_write_hex(uint32_t value, uint8_t color);  // kernel.c에서 정의됨
extern void console_write_dec(uint32_t value, uint8_t color);

// 스택 트레이스 (간단한 구현)
void debug_stack_trace(void) {
    uint8_t color = make_color(0x0E, 0x01);  // 노란색
    
    console_write("Stack trace:\n", color);
    
    // EBP 레지스터 읽기
    uint32_t ebp = 0;
    __asm__ volatile("mov %%ebp, %0" : "=r"(ebp));
    
    console_write("  EBP: ", color);
    console_write_hex(ebp, color);
    console_putc('\n', color);
    
    // 스택 프레임 순회 (최대 10개)
    uint32_t* frame = (uint32_t*)ebp;
    for (int i = 0; i < 10 && frame; ++i) {
        if (frame[1] == 0) break;  // 리턴 주소가 0이면 종료
        
        console_write("  [", color);
        console_write_dec(i, color);
        console_write("] ", color);
        console_write_hex(frame[1], color);  // 리턴 주소
        console_putc('\n', color);
        
        frame = (uint32_t*)frame[0];  // 이전 EBP
        if ((uint32_t)frame < 0x1000 || (uint32_t)frame > 0xFFFFFFFF - 0x1000) {
            break;  // 유효하지 않은 주소
        }
    }
}

// 메모리 덤프
void debug_memory_dump(uint32_t addr, uint32_t size) {
    uint8_t color = make_color(0x0A, 0x01);  // 초록색
    
    console_write("Memory dump at ", color);
    console_write_hex(addr, color);
    console_write(" (", color);
    console_write_dec(size, color);
    console_write(" bytes):\n", color);
    
    uint8_t* ptr = (uint8_t*)addr;
    for (uint32_t i = 0; i < size && i < 256; i += 16) {
        // 주소
        console_write("  ", color);
        console_write_hex(addr + i, color);
        console_write(": ", color);
        
        // 16진수 덤프
        for (uint32_t j = 0; j < 16 && (i + j) < size; ++j) {
            console_write_hex(ptr[i + j], color);
            console_putc(' ', color);
        }
        
        // ASCII 덤프
        console_write(" | ", color);
        for (uint32_t j = 0; j < 16 && (i + j) < size; ++j) {
            char c = ptr[i + j];
            if (c >= 32 && c < 127) {
                console_putc(c, color);
            } else {
                console_putc('.', color);
            }
        }
        console_putc('\n', color);
    }
}

// 레지스터 덤프
void debug_register_dump(void) {
    uint8_t color = make_color(0x0B, 0x01);  // 청록색
    
    uint32_t eax, ebx, ecx, edx, esi, edi, esp, ebp, eip, eflags;
    
    __asm__ volatile("mov %%eax, %0" : "=r"(eax));
    __asm__ volatile("mov %%ebx, %0" : "=r"(ebx));
    __asm__ volatile("mov %%ecx, %0" : "=r"(ecx));
    __asm__ volatile("mov %%edx, %0" : "=r"(edx));
    __asm__ volatile("mov %%esi, %0" : "=r"(esi));
    __asm__ volatile("mov %%edi, %0" : "=r"(edi));
    __asm__ volatile("mov %%esp, %0" : "=r"(esp));
    __asm__ volatile("mov %%ebp, %0" : "=r"(ebp));
    
    // EIP는 직접 읽을 수 없으므로 간단히 0으로 설정
    eip = 0;
    __asm__ volatile("pushf" ::: "memory");
    __asm__ volatile("pop %0" : "=r"(eflags));
    
    console_write("Register dump:\n", color);
    console_write("  EAX: ", color);
    console_write_hex(eax, color);
    console_write("  EBX: ", color);
    console_write_hex(ebx, color);
    console_write("  ECX: ", color);
    console_write_hex(ecx, color);
    console_write("  EDX: ", color);
    console_write_hex(edx, color);
    console_putc('\n', color);
    console_write("  ESI: ", color);
    console_write_hex(esi, color);
    console_write("  EDI: ", color);
    console_write_hex(edi, color);
    console_write("  ESP: ", color);
    console_write_hex(esp, color);
    console_write("  EBP: ", color);
    console_write_hex(ebp, color);
    console_putc('\n', color);
    console_write("  EIP: ", color);
    console_write_hex(eip, color);
    console_write("  EFLAGS: ", color);
    console_write_hex(eflags, color);
    console_putc('\n', color);
}

