// kernel/kernel.c - terraOS 32비트 커널
// VGA 텍스트 콘솔 + 키보드 입력 + 간단 쉘 + IDT/타이머 + 기초 메모리 관리 준비

#include <stdint.h>

static volatile uint16_t* const VIDEO_MEMORY = (uint16_t*)0xB8000;
static const int VGA_WIDTH = 80;
static const int VGA_HEIGHT = 25;

static int cursor_row = 0;
static int cursor_col = 0;

// --- 저수준 포트 I/O -------------------------------------------------------

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" :: "a"(value), "Nd"(port));
}

// --- VGA 텍스트 모드 콘솔 --------------------------------------------------

uint8_t make_color(uint8_t fg, uint8_t bg) {
    return (bg << 4) | (fg & 0x0F);
}

static uint16_t make_vga_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

static void put_char_at(int row, int col, char c, uint8_t color) {
    if (row < 0 || row >= VGA_HEIGHT || col < 0 || col >= VGA_WIDTH) {
        return;
    }
    VIDEO_MEMORY[row * VGA_WIDTH + col] = make_vga_entry(c, color);
}

void clear_screen(uint8_t color) {
    uint16_t blank = make_vga_entry(' ', color);
    for (int y = 0; y < VGA_HEIGHT; ++y) {
        for (int x = 0; x < VGA_WIDTH; ++x) {
            VIDEO_MEMORY[y * VGA_WIDTH + x] = blank;
        }
    }
    cursor_row = 0;
    cursor_col = 0;
}

static void console_newline(void) {
    cursor_col = 0;
    ++cursor_row;
    if (cursor_row >= VGA_HEIGHT) {
        cursor_row = VGA_HEIGHT - 1;
        // 아직은 스크롤을 구현하지 않고 마지막 줄에만 계속 쓴다.
    }
}

void console_putc(char c, uint8_t color) {
    if (c == '\n') {
        console_newline();
        return;
    }

    put_char_at(cursor_row, cursor_col, c, color);
    ++cursor_col;
    if (cursor_col >= VGA_WIDTH) {
        console_newline();
    }
}

void console_write(const char* s, uint8_t color) {
    while (*s) {
        console_putc(*s++, color);
    }
}

// --- 간단한 문자열 유틸리티 (표준 라이브러리 없음) ------------------------

static int str_eq(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

static int str_len(const char* s) {
    int n = 0;
    while (*s++) ++n;
    return n;
}

// 10진수 출력
void console_write_dec(uint32_t value, uint8_t color) {
    char buf[16];
    int i = 0;
    if (value == 0) {
        console_putc('0', color);
        return;
    }
    while (value > 0 && i < (int)sizeof(buf)) {
        buf[i++] = (char)('0' + (value % 10));
        value /= 10;
    }
    while (i-- > 0) {
        console_putc(buf[i], color);
    }
}

// 16진수 출력 (0xXXXXXXXX)
void console_write_hex(uint32_t value, uint8_t color) {
    static const char* digits = "0123456789ABCDEF";
    console_write("0x", color);
    for (int i = 7; i >= 0; --i) {
        uint8_t nibble = (value >> (i * 4)) & 0xF;
        console_putc(digits[nibble], color);
    }
}

// --- 키보드 (폴링 기반) ---------------------------------------------------

// US 키보드 기준, shift/특수키 없는 가장 단순한 스캔코드 → ASCII 매핑
static const char keymap[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b', // 0x0E: Backspace
   '\t', // 0x0F: Tab
    'q','w','e','r','t','y','u','i','o','p','[',']','\n',          // 0x1C: Enter
    0,   // 0x1D: Ctrl
    'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,   // 0x2A: Left Shift
   '\\','z','x','c','v','b','n','m',',','.','/', 
    0,   // 0x36: Right Shift
    '*',
    0,   // Alt
    ' ', // Space
    // 나머지는 0으로 둔다
};

// 한 글자를 읽을 때까지 포트 상태를 폴링한다.
char keyboard_read_char(void) {
    // 먼저 인터럽트 버퍼에서 읽기 시도
    extern int keyboard_read_char_async(char* ch);
    char ch;
    if (keyboard_read_char_async(&ch)) {
        return ch;
    }
    
    // 버퍼가 비어있으면 폴링 (하위 호환성)
    for (;;) {
        // 상태 레지스터(0x64)의 bit0가 1이면 읽을 수 있음
        uint8_t status = inb(0x64);
        if (status & 0x01) {
            uint8_t scancode = inb(0x60);

            // 키가 올라가는 이벤트(0x80 이상)는 무시
            if (scancode & 0x80) {
                continue;
            }

            if (scancode < sizeof(keymap)) {
                char ch = keymap[scancode];
                if (ch != 0) {
                    return ch;
                }
            }
        }
        
        // 인터럽트 버퍼도 다시 확인
        if (keyboard_read_char_async(&ch)) {
            return ch;
        }
    }
}

// 한 줄 입력 (엔터까지). 최대 max_len-1 글자, 마지막에 '\0'
static void keyboard_read_line(char* buffer, int max_len, uint8_t color) {
    int len = 0;
    while (1) {
        char c = keyboard_read_char();

        if (c == '\n') {
            console_putc('\n', color);
            break;
        }

        if (c == '\b') { // 백스페이스
            if (len > 0) {
                // 화면 상에서도 한 글자 지우기
                --len;
                if (cursor_col > 0) {
                    --cursor_col;
                    put_char_at(cursor_row, cursor_col, ' ', color);
                }
            }
            continue;
        }

        if (len < max_len - 1) {
            buffer[len++] = c;
            console_putc(c, color);
        }
    }
    buffer[len] = '\0';
}

// --- IDT / PIC / PIT ------------------------------------------------------

struct idt_entry {
    uint16_t base_low;
    uint16_t sel;
    uint8_t  always0;
    uint8_t  flags;
    uint16_t base_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idt_ptr   idtp;

extern void isr0(void);
extern void isr13(void);
extern void isr14(void);
extern void isr128(void);  // syscall
extern void irq0(void);
extern void irq1(void);

volatile uint64_t timer_ticks = 0;  // time.c에서 접근 필요

static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = (uint16_t)(base & 0xFFFF);
    idt[num].base_high = (uint16_t)((base >> 16) & 0xFFFF);
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

static void idt_install(void) {
    idtp.limit = sizeof(struct idt_entry) * 256 - 1;
    idtp.base  = (uint32_t)&idt;

    for (int i = 0; i < 256; ++i) {
        idt_set_gate((uint8_t)i, 0, 0, 0);
    }

    // 0x08: 커널 코드 세그먼트, 0x8E: present, ring0, 32비트 interrupt gate
    idt_set_gate(0,  (uint32_t)isr0,  0x08, 0x8E);
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
    
    // 시스템 콜 인터럽트 (0x80) - 유저 모드에서도 호출 가능하도록 설정
    // 0xEE: present, ring3, 32비트 interrupt gate (유저 모드에서 호출 가능)
    idt_set_gate(128, (uint32_t)isr128, 0x08, 0xEE);

    // PIC 리맵 후 IRQ0, IRQ1 (타이머, 키보드)
    idt_set_gate(32, (uint32_t)irq0, 0x08, 0x8E);
    idt_set_gate(33, (uint32_t)irq1, 0x08, 0x8E);

    __asm__ volatile("lidt (%0)" :: "r"(&idtp));
}

static void pic_remap(void) {
    // PIC 재설정 (표준 시퀀스)
    uint8_t a1 = inb(0x21);
    uint8_t a2 = inb(0xA1);

    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20); // Master offset 0x20
    outb(0xA1, 0x28); // Slave offset 0x28
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    // 모든 IRQ 허용 (나중에 마스크 조정 가능)
    outb(0x21, a1 & ~(1 << 0)); // 타이머(IRQ0) 허용
    outb(0xA1, a2);
}

static void pit_init(uint32_t frequency) {
    uint32_t divisor = 1193180 / frequency;
    outb(0x43, 0x36);                // 모드 3, 채널 0, LSB/MSB
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}

// --- Multiboot 메모리 정보 + 간단한 커널 힙 -------------------------------

// linker.ld 에서 정의한 커널 끝 심볼
extern uint32_t _kernel_end;

typedef struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    // 나머지 필드는 지금은 사용하지 않는다.
} multiboot_info_t;

static uint32_t total_mem_kb = 0;
static uint32_t heap_start = 0;
static uint32_t heap_end   = 0;
static uint32_t heap_curr  = 0;
static uint32_t heap_size_bytes = 0;

static void mem_init(multiboot_info_t* mb_info) {
    uint8_t info_color = make_color(0x0F, 0x01);

    if (mb_info && (mb_info->flags & 0x1)) {
        total_mem_kb = mb_info->mem_lower + mb_info->mem_upper;
    } else {
        total_mem_kb = 0;
    }

    uint32_t kend = (uint32_t)&_kernel_end;
    heap_start = (kend + 0xFFF) & ~0xFFF;   // 4KB 정렬

    // 일단 1 MiB를 힙으로 사용 (나중에 동적으로 조정 가능)
    heap_size_bytes = 1024 * 1024;
    heap_end = heap_start + heap_size_bytes;
    heap_curr = heap_start;

    console_write("Memory info: total=", info_color);
    console_write_dec(total_mem_kb, info_color);
    console_write(" KB, kernel_end=", info_color);
    console_write_hex(kend, info_color);
    console_write(", heap_start=", info_color);
    console_write_hex(heap_start, info_color);
    console_write(", heap_size=", info_color);
    console_write_dec(heap_size_bytes / 1024, info_color);
    console_write(" KB\n", info_color);
}

void* kmalloc(uint32_t size) {
    extern void* heap_alloc(uint32_t size);
    return heap_alloc(size);
}

void kfree(void* ptr) {
    extern void heap_free(void* ptr);
    heap_free(ptr);
}

// --- 페이징 (페이지 단위 메모리 관리 진입점) ------------------------------

extern void paging_init(void);

// --- GDT 초기화 -----------------------------------------------------------

extern void gdt_init(void);

// --- RAM 디스크 파일 시스템 (ramfs) ---------------------------------------

// ramfs_file_t는 ramfs.c에 정의되어 있음
struct ramfs_file {
    const char* name;
    const char* data;
    uint32_t    size;
    int         is_dir;
    int         writable;
    uint32_t    permissions;
    uint32_t    owner;
    uint32_t    group;
};
typedef struct ramfs_file ramfs_file_t;
extern uint32_t ramfs_list(ramfs_file_t** out_files);
extern int ramfs_read(const char* name, const char** out_data, uint32_t* out_size);

// --- 선점형 멀티태스킹 (tasks.c에서) ----------------------------------------

extern void schedule(void);
extern void run_preemptive_demo(void);
extern void task_print_info(void);

// --- 협력형 멀티태스킹 (Cooperative Multitasking) --------------------------

typedef enum {
    TASK_STATE_READY,
    TASK_STATE_RUNNING,
    TASK_STATE_DONE
} task_state_t;

typedef struct {
    const char*     name;
    void            (*func)(void);
    uint32_t        counter;
    task_state_t    state;
} coop_task_t;

#define MAX_TASKS 8
static coop_task_t tasks[MAX_TASKS];
static int num_tasks = 0;
static int current_task_idx = -1;
static int coop_demo_running = 0;

// yield() - 현재 태스크가 제어권을 반환
static void yield(void) {
    if (!coop_demo_running) return;
    
    // 다음 태스크로 전환
    int next_idx = (current_task_idx + 1) % num_tasks;
    while (tasks[next_idx].state == TASK_STATE_DONE) {
        next_idx = (next_idx + 1) % num_tasks;
        // 모든 태스크가 끝났는지 확인
        int all_done = 1;
        for (int i = 0; i < num_tasks; ++i) {
            if (tasks[i].state != TASK_STATE_DONE) {
                all_done = 0;
                break;
            }
        }
        if (all_done) {
            coop_demo_running = 0;
            return;
        }
    }
    
    current_task_idx = next_idx;
}

// 태스크 추가
static int task_add(const char* name, void (*func)(void)) {
    if (num_tasks >= MAX_TASKS) return 0;
    
    tasks[num_tasks].name = name;
    tasks[num_tasks].func = func;
    tasks[num_tasks].counter = 0;
    tasks[num_tasks].state = TASK_STATE_READY;
    ++num_tasks;
    return 1;
}

// 데모 태스크 A
static void taskA(void) {
    uint8_t color = make_color(0x0E, 0x01); // 노란색
    console_write("[TaskA #", color);
    console_write_dec(tasks[0].counter, color);
    console_write("] ", color);
    
    ++tasks[0].counter;
    
    // 10번 실행 후 종료
    if (tasks[0].counter >= 10) {
        tasks[0].state = TASK_STATE_DONE;
        console_write("TaskA finished!\n", color);
        return;
    }
    
    yield(); // 제어권 반환
}

// 데모 태스크 B
static void taskB(void) {
    uint8_t color = make_color(0x0B, 0x01); // 청록색
    console_write("[TaskB #", color);
    console_write_dec(tasks[1].counter, color);
    console_write("] ", color);
    
    ++tasks[1].counter;
    
    // 10번 실행 후 종료
    if (tasks[1].counter >= 10) {
        tasks[1].state = TASK_STATE_DONE;
        console_write("TaskB finished!\n", color);
        return;
    }
    
    yield(); // 제어권 반환
}

// 협력형 멀티태스킹 데모 실행
void run_coop_demo(void) {
    uint8_t color = make_color(0x0F, 0x00);
    
    // 태스크 초기화
    num_tasks = 0;
    current_task_idx = -1;
    
    task_add("TaskA", taskA);
    task_add("TaskB", taskB);
    
    console_write("\n=== Cooperative Multitasking Demo ===\n", color);
    console_write("Two tasks will run cooperatively, yielding control to each other.\n", color);
    console_write("Each task will run 10 times, then finish.\n\n", color);
    
    coop_demo_running = 1;
    current_task_idx = 0;
    
    // 태스크 루프 실행
    while (coop_demo_running) {
        if (current_task_idx >= 0 && current_task_idx < num_tasks) {
            coop_task_t* task = &tasks[current_task_idx];
            if (task->state == TASK_STATE_READY || task->state == TASK_STATE_RUNNING) {
                task->state = TASK_STATE_RUNNING;
                task->func(); // 태스크 실행 (내부에서 yield() 호출)
            }
        } else {
            coop_demo_running = 0;
        }
    }
    
    console_write("\n=== Demo finished ===\n", color);
}

// 시스템 콜 핸들러 (syscall.c에서)
extern uint32_t syscall_handler(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3);

// 시스템 콜 핸들러 C 래퍼 (어셈블리에서 호출)
uint32_t syscall_handler_c(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    return syscall_handler(syscall_num, arg1, arg2, arg3);
}

// ISR/IRQ 공통 C 핸들러
void isr_handler(uint32_t int_no, uint32_t err_code) {
    (void)err_code;
    
    // 시스템 콜은 어셈블리에서 직접 처리하므로 여기서는 처리하지 않음
    if (int_no == 128) {
        return;
    }
    
    // 예외 처리
    console_write("Exception: ", make_color(0x0C, 0x01));
    if (int_no == 0) {
        console_write("Divide by zero\n", make_color(0x0C, 0x01));
    } else if (int_no == 13) {
        // General protection fault
    } else if (int_no == 14) {
        // Page fault
    } else {
        console_write("Unknown\n", make_color(0x0C, 0x01));
    }
    for (;;) {
        __asm__ volatile("hlt");
    }
}

// 키보드 인터럽트 핸들러 (keyboard.c에서)
extern void keyboard_irq_handler(void);

void irq_handler(uint32_t int_no, uint32_t err_code) {
    (void)err_code;

    if (int_no == 32) { // 타이머
        ++timer_ticks;
        // 시간 업데이트
        extern void time_update(void);
        time_update();
        
        // 너무 자주 찍으면 화면이 망가져서, 간단히 1초마다 한 번 정도만
        if (timer_ticks % 100 == 0) {
            console_write("[tick] ", make_color(0x0A, 0x01));
        }
        
        // 선점형 멀티태스킹: 타이머 인터럽트마다 스케줄러 호출
        schedule();
    } else if (int_no == 33) {
        keyboard_irq_handler();
    }

    // EOI (End of interrupt) - 마스터 PIC에만 전송 (IRQ0,1)
    outb(0x20, 0x20);
}

// Multiboot 진입점: boot.asm의 _start에서 eax, ebx를 넘겨준다.
void kmain(uint32_t multiboot_magic, uint32_t multiboot_info_addr) {
    // 새로운 디자인 시스템 사용
    extern void design_boot_screen(void);
    extern void design_success(const char* message);
    extern void design_warning(const char* message);
    extern void design_info(const char* message);
    
    design_boot_screen();

    if (multiboot_magic != 0x2BADB002) {
        design_warning("Invalid multiboot magic!");
    }

    design_info("Setting up GDT, IDT, PIC, PIT timer, paging...");

    // GDT 초기화 (유저 모드 세그먼트 포함)
    gdt_init();
    
    idt_install();
    pic_remap();
    pit_init(100); // 100Hz

    // 메모리 / 힙 초기화
    mem_init((multiboot_info_t*)multiboot_info_addr);
    
    // 프레임 할당자 초기화
    extern void frame_init(uint32_t total_memory_kb);
    frame_init(total_mem_kb);

    // 간단한 4MiB identity paging 활성화
    paging_init();
    
    // 키보드 인터럽트 초기화
    extern void keyboard_init(void);
    keyboard_init();
    
    // RAM 디스크 초기화 (ELF 파일 포함)
    extern void ramfs_init(void);
    ramfs_init();
    
    // VFS 초기화
    extern void vfs_init(void);
    vfs_init();
    
    // 로깅 시스템 초기화
    extern void klog_init(void);
    klog_init();
    
    // 환경 변수 초기화
    extern void env_init(void);
    env_init();
    
    // 시그널 시스템 초기화
    extern void signal_init(void);
    signal_init();
    
    // 파이프 초기화
    extern void pipe_init(void);
    pipe_init();
    
    // 네트워크 초기화
    extern void network_init(void);
    network_init();
    
    // SMP 초기화
    extern void smp_init(void);
    smp_init();
    
    // 그래픽 초기화
    extern void graphics_init(uint32_t multiboot_info_addr);
    graphics_init(multiboot_info_addr);
    
    // 쉘 초기화
    extern void shell_init(void);
    shell_init();
    
    // 디바이스 파일 시스템 초기화
    extern void devfs_init(void);
    devfs_init();
    
    // 개선된 힙 초기화
    extern void heap_init(uint32_t base, uint32_t size);
    heap_init(heap_start, heap_size_bytes);

    __asm__ volatile("sti"); // 인터럽트 활성화

    design_success("System initialized successfully!");
    design_info("Shell ready. Type 'help' for commands.");
    console_putc('\n', make_color(0x0F, 0x00));

    char line[80];

    uint8_t default_color = make_color(0x0F, 0x00);
    
    while (1) {
        extern void design_print_prompt(void);
        design_print_prompt();
        keyboard_read_line(line, sizeof(line), default_color);

        if (str_eq(line, "help")) {
            extern void design_print_box(const char* title);
            extern void design_close_box(void);
            design_print_box("Available Commands");
            uint8_t color = make_color(0x0F, 0x00);
            uint8_t fg_color = color;
            console_write("  help      - show this help\n", color);
            console_write("  clear     - clear the screen\n", color);
            console_write("  echo X    - echo back text X\n", color);
            console_write("  meminfo   - show basic memory info\n", color);
            console_write("  alloc     - allocate 64 bytes from kernel heap\n", color);
            console_write("  ls        - list files in RAMFS\n", color);
            console_write("  cat F     - print file F from RAMFS\n", color);
            console_write("  demo-coop  - run cooperative multitasking demo\n", color);
            console_write("  demo-preempt - run preemptive multitasking demo\n", color);
            console_write("  demo-usermode - run user mode (Ring 3) demo\n", color);
            console_write("  ps        - show task list\n", color);
            console_write("  syscall-test - test system call interface\n", color);
            console_write("  run FILE  - execute ELF file from RAMFS\n", color);
            console_write("  memmap    - show memory mapping info\n", color);
            console_write("  time      - show system uptime\n", color);
            console_write("  log       - show recent kernel logs\n", color);
            console_write("  export VAR=value - set environment variable\n", color);
            console_write("  env       - show all environment variables\n", color);
            console_write("  mkdir DIR - create directory\n", color);
            console_write("  rm FILE   - remove file\n", color);
            console_write("  nice PID VALUE - set process priority\n", color);
            console_write("  kill PID  - send signal to process\n", color);
            console_write("  cpuinfo   - show CPU information\n", color);
            console_write("  jobs      - show background jobs\n", color);
            console_write("  fg JOB    - bring job to foreground\n", color);
            console_write("  bg JOB    - run job in background\n", color);
            console_write("  cd DIR    - change directory\n", color);
            console_write("  pwd       - print working directory\n", color);
            console_write("  dev       - list devices in /dev\n", color);
            console_write("  heapinfo  - show heap allocation info\n", color);
            console_write("  stacktrace - show stack trace\n", color);
            console_write("  memdump ADDR SIZE - dump memory\n", color);
            console_write("  regs      - show register dump\n", color);
            console_write("  top       - show process monitor\n", color);
            console_write("  vmstat    - show system statistics\n", fg_color);
            design_close_box();
        } else if (str_eq(line, "clear")) {
            uint8_t clear_color = make_color(0x00, 0x00);
            clear_screen(clear_color);
        } else if (str_eq(line, "meminfo")) {
            uint8_t color = default_color;
            console_write("Total memory: ", color);
            console_write_dec(total_mem_kb, color);
            console_write(" KB\n", color);
            console_write("Heap start: ", color);
            console_write_hex(heap_start, color);
            console_write("\nHeap end:   ", color);
            console_write_hex(heap_end, color);
            console_write("\nHeap used:  ", color);
            console_write_dec((heap_curr - heap_start) / 1024, color);
            console_write(" KB / ", color);
            console_write_dec(heap_size_bytes / 1024, color);
            console_write(" KB\n", color);
        } else if (str_eq(line, "memmap")) {
            extern void memory_get_info(uint32_t* total, uint32_t* used);
            uint32_t total_mem = 0, used_mem = 0;
            memory_get_info(&total_mem, &used_mem);
            uint8_t color = default_color;
            console_write("Frame allocator:\n", color);
            console_write("  Total frames: ", color);
            console_write_dec(total_mem / 4096, color);
            console_write("\n  Used frames:  ", color);
            console_write_dec(used_mem / 4096, color);
            console_write("\n  Free frames:  ", color);
            console_write_dec((total_mem - used_mem) / 4096, color);
            console_putc('\n', color);
        } else if (str_eq(line, "time")) {
            uint8_t color = default_color;
            extern uint32_t time_get_seconds(void);
            extern void time_format_seconds(uint32_t seconds, char* buffer, int buffer_size);
            uint32_t seconds = time_get_seconds();
            char time_str[16];
            time_format_seconds(seconds, time_str, sizeof(time_str));
            console_write("System uptime: ", color);
            console_write(time_str, color);
            console_write(" (", color);
            console_write_dec(seconds, color);
            console_write(" seconds)\n", color);
        } else if (str_eq(line, "log")) {
            uint8_t color = default_color;
            extern uint32_t klog_get_entries(char** out_entries, uint32_t max_count);
            char* entries[32];
            uint32_t count = klog_get_entries(entries, 32);
            console_write("Recent kernel logs (", color);
            console_write_dec(count, color);
            console_write(" entries):\n", color);
            for (uint32_t i = 0; i < count; ++i) {
                console_write("  ", color);
                console_write(entries[i], color);
                console_putc('\n', color);
            }
        } else if (str_eq(line, "env")) {
            uint8_t color = default_color;
            typedef struct { char name[64]; char value[256]; int valid; } env_var_t;
            extern int env_list(env_var_t** out_vars);
            env_var_t* vars = 0;
            int count = env_list(&vars);
            console_write("Environment variables:\n", color);
            for (int i = 0; i < count; ++i) {
                if (vars[i].valid) {
                    console_write("  ", color);
                    console_write(vars[i].name, color);
                    console_write("=", color);
                    console_write(vars[i].value, color);
                    console_putc('\n', color);
                }
            }
        } else if (str_len(line) > 7 &&
                   line[0]=='e' && line[1]=='x' && line[2]=='p' &&
                   line[3]=='o' && line[4]=='r' && line[5]=='t' &&
                   line[6]==' ') {
            uint8_t color = default_color;
            const char* arg = line + 7;
            // VAR=value 형식 파싱
            int eq_pos = -1;
            for (int i = 0; arg[i]; ++i) {
                if (arg[i] == '=') {
                    eq_pos = i;
                    break;
                }
            }
            if (eq_pos > 0) {
                char var_name[64] = {0};
                char var_value[256] = {0};
                for (int i = 0; i < eq_pos && i < 63; ++i) {
                    var_name[i] = arg[i];
                }
                for (int i = eq_pos + 1; arg[i] && (i - eq_pos - 1) < 255; ++i) {
                    var_value[i - eq_pos - 1] = arg[i];
                }
                extern int env_set(const char* name, const char* value);
                if (env_set(var_name, var_value) == 0) {
                    console_write("Environment variable set: ", color);
                    console_write(var_name, color);
                    console_write("=", color);
                    console_write(var_value, color);
                    console_putc('\n', color);
                } else {
                    console_write("Failed to set environment variable\n", color);
                }
            } else {
                console_write("Usage: export VAR=value\n", color);
            }
        } else if (str_len(line) > 6 &&
                   line[0]=='m' && line[1]=='k' && line[2]=='d' &&
                   line[3]=='i' && line[4]=='r' && line[5]==' ') {
            uint8_t color = default_color;
            const char* dirname = line + 6;
            extern int ramfs_create(const char* name, int is_dir);
            if (ramfs_create(dirname, 1)) {
                console_write("Directory created: ", color);
                console_write(dirname, color);
                console_putc('\n', color);
            } else {
                console_write("Failed to create directory\n", color);
            }
        } else if (str_len(line) > 3 &&
                   line[0]=='r' && line[1]=='m' && line[2]==' ') {
            uint8_t color = default_color;
            const char* filename = line + 3;
            extern int ramfs_delete(const char* name);
            if (ramfs_delete(filename)) {
                console_write("File deleted: ", color);
                console_write(filename, color);
                console_putc('\n', color);
            } else {
                console_write("Failed to delete file\n", color);
            }
        } else if (str_len(line) > 5 &&
                   line[3]=='e' && line[4]==' ') {
            uint8_t color = default_color;
            const char* args = line + 5;
            uint32_t pid = 0, nice_val = 0;
            int parsing_pid = 1;
            for (int i = 0; args[i]; ++i) {
                if (args[i] == ' ') {
                    parsing_pid = 0;
                    continue;
                }
                if (parsing_pid && args[i] >= '0' && args[i] <= '9') {
                    pid = pid * 10 + (args[i] - '0');
                } else if (!parsing_pid && args[i] >= '0' && args[i] <= '9') {
                } else if (!parsing_pid && args[i] == '-') {
                }
            }
            extern void task_set_priority(uint32_t pid, int nice);
            task_set_priority(pid, (int)nice_val);
            console_write("Priority set for PID ", color);
            console_write_dec(pid, color);
            console_write(" to nice=", color);
            console_write_dec(nice_val, color);
            console_putc('\n', color);
        } else if (str_len(line) > 5 &&
                   line[3]=='l' && line[4]==' ') {
            uint8_t color = default_color;
            const char* args = line + 5;
            uint32_t pid = 0;
            for (int i = 0; args[i] && args[i] >= '0' && args[i] <= '9'; ++i) {
                pid = pid * 10 + (args[i] - '0');
            }
            extern int signal_send(uint32_t pid, int sig);
            if (signal_send(pid, 15) == 0) {  // SIGTERM
                console_write("Signal sent to PID ", color);
                console_write_dec(pid, color);
                console_putc('\n', color);
            } else {
                console_write("Failed to send signal\n", color);
            }
        } else if (str_eq(line, "cpuinfo")) {
            uint8_t color = default_color;
            extern int smp_get_cpu_count(void);
            extern int smp_get_current_cpu(void);
            int cpu_count = smp_get_cpu_count();
            int current_cpu = smp_get_current_cpu();
            console_write("CPU Information:\n", color);
            console_write("  CPUs: ", color);
            console_write_dec(cpu_count, color);
            console_write("\n  Current CPU: ", color);
            console_write_dec(current_cpu, color);
            console_putc('\n', color);
        } else if (str_eq(line, "jobs")) {
            uint8_t color = default_color;
            typedef struct { uint32_t pid; char command[128]; int valid; } job_t;
            extern int shell_list_jobs(job_t** out_jobs);
            job_t* jobs = 0;
            int count = shell_list_jobs(&jobs);
            console_write("Background jobs:\n", color);
            int job_num = 1;
            for (int i = 0; i < count; ++i) {
                if (jobs[i].valid) {
                    console_write("  [", color);
                    console_write_dec(job_num++, color);
                    console_write("] PID ", color);
                    console_write_dec(jobs[i].pid, color);
                    console_write(": ", color);
                    console_write(jobs[i].command, color);
                    console_putc('\n', color);
                }
            }
            if (job_num == 1) {
                console_write("  No background jobs\n", color);
            }
        } else if (str_len(line) > 3 &&
                   line[0]=='f' && line[1]=='g' && line[2]==' ') {
            uint8_t color = default_color;
            // 간단히 구현: PID를 포그라운드로
            const char* arg = line + 3;
            uint32_t job_id = 0;
            for (int i = 0; arg[i] && arg[i] >= '0' && arg[i] <= '9'; ++i) {
                job_id = job_id * 10 + (arg[i] - '0');
            }
            console_write("Job ", color);
            console_write_dec(job_id, color);
            console_write(" brought to foreground\n", color);
        } else if (str_len(line) > 3 &&
                   line[0]=='b' && line[1]=='g' && line[2]==' ') {
            uint8_t color = default_color;
            // 간단히 구현
            const char* arg = line + 3;
            uint32_t job_id = 0;
            for (int i = 0; arg[i] && arg[i] >= '0' && arg[i] <= '9'; ++i) {
                job_id = job_id * 10 + (arg[i] - '0');
            }
            console_write("Job ", color);
            console_write_dec(job_id, color);
            console_write(" running in background\n", color);
        } else if (str_len(line) > 3 &&
                   line[0]=='c' && line[1]=='d' && line[2]==' ') {
            uint8_t color = default_color;
            const char* dirname = line + 3;
            extern int path_chdir(const char* path);
            if (path_chdir(dirname) == 0) {
                extern const char* path_getcwd(void);
                console_write("Changed directory to: ", color);
                console_write(path_getcwd(), color);
                console_putc('\n', color);
            } else {
                console_write("Failed to change directory\n", color);
            }
        } else if (str_eq(line, "pwd")) {
            uint8_t color = default_color;
            extern const char* path_getcwd(void);
            const char* cwd = path_getcwd();
            console_write(cwd ? cwd : "/", color);
            console_putc('\n', color);
        } else if (str_eq(line, "dev")) {
            extern void devfs_list_devices(void);
            devfs_list_devices();
        } else if (str_eq(line, "heapinfo")) {
            uint8_t color = default_color;
            extern void heap_get_info(uint32_t* total, uint32_t* used, uint32_t* free);
            uint32_t total = 0, used = 0, free = 0;
            heap_get_info(&total, &used, &free);
            console_write("Heap information:\n", color);
            console_write("  Total: ", color);
            console_write_dec(total / 1024, color);
            console_write(" KB\n", color);
            console_write("  Used:  ", color);
            console_write_dec(used / 1024, color);
            console_write(" KB\n", color);
            console_write("  Free:  ", color);
            console_write_dec(free / 1024, color);
            console_write(" KB\n", color);
        } else if (str_eq(line, "stacktrace")) {
            extern void debug_stack_trace(void);
            debug_stack_trace();
        } else if (str_len(line) > 8 &&
                   line[0]=='m' && line[1]=='e' && line[2]=='m' &&
                   line[3]=='d' && line[4]=='u' && line[5]=='m' &&
                   line[6]=='p' && line[7]==' ') {
            const char* args = line + 8;
            uint32_t addr = 0, size = 0;
            int parsing_addr = 1;
            for (int i = 0; args[i]; ++i) {
                if (args[i] == ' ') {
                    parsing_addr = 0;
                    continue;
                }
                if (args[i] >= '0' && args[i] <= '9') {
                    if (parsing_addr) {
                        addr = addr * 10 + (args[i] - '0');
                    } else {
                        size = size * 10 + (args[i] - '0');
                    }
                } else if (args[i] >= 'a' && args[i] <= 'f') {
                    if (parsing_addr) {
                        addr = addr * 16 + (args[i] - 'a' + 10);
                    } else {
                        size = size * 16 + (args[i] - 'a' + 10);
                    }
                } else if (args[i] >= 'A' && args[i] <= 'F') {
                    if (parsing_addr) {
                        addr = addr * 16 + (args[i] - 'A' + 10);
                    } else {
                        size = size * 16 + (args[i] - 'A' + 10);
                    }
                }
            }
            if (size == 0) size = 64;  // 기본값
            extern void debug_memory_dump(uint32_t addr, uint32_t size);
            debug_memory_dump(addr, size);
        } else if (str_eq(line, "regs")) {
            extern void debug_register_dump(void);
            debug_register_dump();
        } else if (str_eq(line, "top")) {
            extern void monitor_top(void);
            monitor_top();
        } else if (str_eq(line, "vmstat")) {
            extern void monitor_vmstat(void);
            monitor_vmstat();
        } else if (str_eq(line, "alloc")) {
            uint8_t color = default_color;
            extern void* kmalloc(uint32_t size);
            void* p = kmalloc(64);
            if (p) {
                console_write("Allocated 64 bytes at ", color);
                console_write_hex((uint32_t)p, color);
                console_putc('\n', color);
            } else {
                console_write("Allocation failed (out of heap)\n", color);
            }
        } else if (str_eq(line, "ls")) {
            extern uint32_t ramfs_list(ramfs_file_t** out_files);
            ramfs_file_t* files = 0;
            uint32_t count = ramfs_list(&files);
            extern void design_print_box(const char* title);
            extern void design_close_box(void);
            design_print_box("Files");
            uint8_t fg = make_color(0x0F, 0x00);
            uint8_t accent = make_color(0x0B, 0x00);
            for (uint32_t i = 0; i < count; ++i) {
                console_write("│ ", accent);
                console_write(files[i].name, fg);
                console_write(" (", fg);
                console_write_dec(files[i].size, fg);
                console_write(" bytes)\n", fg);
            }
            design_close_box();
        } else if (str_len(line) > 4 &&
                   line[0]=='c' && line[1]=='a' && line[2]=='t' &&
                   line[3]==' ') {
            uint8_t color = default_color;
            const char* filename = line + 4;
            const char* data = 0;
            uint32_t size = 0;
            if (ramfs_read(filename, &data, &size)) {
                for (uint32_t i = 0; i < size; ++i) {
                    console_putc(data[i], color);
                }
                if (size == 0 || data[size-1] != '\n') {
                    console_putc('\n', color);
                }
            } else {
                console_write("File not found: ", color);
                console_write(filename, color);
                console_putc('\n', color);
            }
        } else if (str_len(line) > 5 &&
                   line[0]=='e' && line[1]=='c' && line[2]=='h' &&
                   line[3]=='o' && line[4]==' ') {
            uint8_t color = default_color;
            console_write("echo: ", color);
            console_write(line + 5, color);
            console_putc('\n', color);
        } else if (str_eq(line, "demo-coop")) {
            run_coop_demo();
        } else if (str_eq(line, "demo-preempt")) {
            run_preemptive_demo();
        } else if (str_eq(line, "demo-usermode")) {
            extern void run_usermode_demo(void);
            run_usermode_demo();
        } else if (str_eq(line, "ps")) {
            task_print_info();
        } else if (str_len(line) > 4 &&
                   line[0]=='r' && line[1]=='u' && line[2]=='n' &&
                   line[3]==' ') {
            uint8_t color = default_color;
            const char* filename = line + 4;
            extern int elf_load_and_execute(const char* filename);
            int result = elf_load_and_execute(filename);
            if (result == 0) {
                console_write("ELF file loaded and started.\n", color);
            } else {
                console_write("Failed to load ELF file. Error code: ", color);
                console_write_dec((uint32_t)result, color);
                console_putc('\n', color);
            }
        } else if (str_eq(line, "syscall-test")) {
            uint8_t color = make_color(0x0F, 0x01);
            console_write("Testing system calls...\n", color);
            
            // syscall_handler 직접 호출 (테스트용)
            extern uint32_t syscall_handler(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3);
            
            console_write("Testing SYS_WRITE (1): ", color);
            syscall_handler(1, 1, (uint32_t)"Hello from syscall!\n", 20);
            
            console_write("Testing SYS_GETPID (5): PID=", color);
            uint32_t pid = syscall_handler(5, 0, 0, 0);
            console_write_dec(pid, color);
            console_putc('\n', color);
            
            console_write("System call test completed.\n", color);
        } else if (str_eq(line, "")) {
            // 빈 줄은 무시
        } else {
            // 고급 쉘 파싱 시도 (파이프, 리다이렉션, 백그라운드)
            // shell.c의 command_t 구조체 사용
            extern int shell_parse_command(const char* line, void* cmd);
            
            // command_t와 동일한 구조체 정의
            struct {
                char tokens[16][64];
                int token_count;
                int has_pipe;
                int has_redirect_in;
                int has_redirect_out;
                int background;
                char redirect_in_file[64];
                char redirect_out_file[64];
            } cmd;
            
            if (shell_parse_command(line, &cmd) == 0 && cmd.token_count > 0) {
                // 파이프나 리다이렉션이 있으면 처리
                if (cmd.has_pipe) {
                    // 파이프 실행
                    extern int exec_pipe(const char* cmd1, const char* arg1, const char* cmd2, const char* arg2);
                    const char* arg1 = cmd.token_count > 1 ? cmd.tokens[1] : 0;
                    const char* arg2 = cmd.token_count > 2 ? cmd.tokens[2] : 0;
                    exec_pipe(cmd.tokens[0], arg1, cmd.tokens[1], arg2);
                } else if (cmd.has_redirect_out) {
                    extern int exec_redirect(const char* cmd, const char* arg, const char* file, int is_output);
                    const char* arg = cmd.token_count > 1 ? cmd.tokens[1] : 0;
                    exec_redirect(cmd.tokens[0], arg, cmd.redirect_out_file, 1);
                } else if (cmd.has_redirect_in) {
                    extern int exec_redirect(const char* cmd, const char* arg, const char* file, int is_output);
                    exec_redirect(cmd.tokens[0], 0, cmd.redirect_in_file, 0);
                } else if (cmd.background) {
                    uint8_t color = default_color;
                    console_write("Background execution: ", color);
                    console_write(cmd.tokens[0], color);
                    console_putc('\n', color);
                    // 백그라운드 작업으로 추가
                    extern int shell_add_job(uint32_t pid, const char* command);
                    shell_add_job(0, cmd.tokens[0]);  // 간단히 PID 0으로
                } else {
                    // 일반 명령어는 기존 방식으로 처리
                    uint8_t color = default_color;
                    console_write("Unknown command: ", color);
                    console_write(line, color);
                    console_putc('\n', color);
                }
            } else {
                uint8_t color = default_color;
                console_write("Unknown command: ", color);
                console_write(line, color);
                console_putc('\n', color);
            }
        }
    }
}

