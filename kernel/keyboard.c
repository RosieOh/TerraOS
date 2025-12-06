// kernel/keyboard.c - 키보드 인터럽트 드라이버
// IRQ1 인터럽트 기반 키보드 입력 처리

#include <stdint.h>

// 키보드 입력 버퍼
#define KEYBOARD_BUFFER_SIZE 256
static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static volatile uint32_t keyboard_read_pos = 0;
static volatile uint32_t keyboard_write_pos = 0;
static volatile uint32_t keyboard_count = 0;

// US 키보드 스캔코드 → ASCII 매핑
static const char keymap[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
   '\t', 'q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,   'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,   '\\','z','x','c','v','b','n','m',',','.', '/',
    0,   '*',
    0,   ' ', // Space
};

// 포트 I/O
static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" :: "a"(value), "Nd"(port));
}

// 키보드 인터럽트 핸들러
void keyboard_irq_handler(void) {
    // 키보드 상태 확인
    uint8_t status = inb(0x64);
    if (!(status & 0x01)) return;  // 읽을 데이터 없음
    
    uint8_t scancode = inb(0x60);
    
    // 키 릴리스 이벤트 무시
    if (scancode & 0x80) return;
    
    // 스캔코드 → ASCII 변환
    if (scancode < sizeof(keymap)) {
        char ch = keymap[scancode];
        if (ch != 0) {
            // 버퍼에 추가
            if (keyboard_count < KEYBOARD_BUFFER_SIZE) {
                keyboard_buffer[keyboard_write_pos] = ch;
                keyboard_write_pos = (keyboard_write_pos + 1) % KEYBOARD_BUFFER_SIZE;
                ++keyboard_count;
            }
        }
    }
}

// 키보드에서 문자 읽기 (비동기)
int keyboard_read_char_async(char* ch) {
    if (keyboard_count == 0) {
        return 0;  // 버퍼 비어있음
    }
    
    *ch = keyboard_buffer[keyboard_read_pos];
    keyboard_read_pos = (keyboard_read_pos + 1) % KEYBOARD_BUFFER_SIZE;
    --keyboard_count;
    return 1;
}

// 키보드 초기화
void keyboard_init(void) {
    keyboard_read_pos = 0;
    keyboard_write_pos = 0;
    keyboard_count = 0;
    
    // 키보드 인터럽트 활성화 (PIC 마스크)
    uint8_t mask = inb(0x21);
    outb(0x21, mask & ~(1 << 1));  // IRQ1 (키보드) 허용
}

