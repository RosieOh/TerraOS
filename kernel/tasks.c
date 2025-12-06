// kernel/tasks.c - 선점형 멀티태스킹 (Preemptive Multitasking)
// 타이머 인터럽트 기반 태스크 스위칭

#include <stdint.h>

// 태스크 컨텍스트 구조체 (레지스터 저장용)
typedef struct {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;  // pusha로 저장되는 레지스터
    uint32_t eip;  // 명시적으로 저장
    uint32_t eflags;
} task_context_t;

// 태스크 상태
typedef enum {
    TASK_STATE_READY = 0,
    TASK_STATE_RUNNING = 1,
    TASK_STATE_SLEEPING = 2,
    TASK_STATE_DONE = 3
} task_state_t;

// 태스크 구조체
typedef struct task {
    const char*     name;
    void            (*entry)(void);  // 태스크 진입점 함수
    uint32_t        stack_top;       // 스택 상단 주소
    uint32_t        stack_size;      // 스택 크기
    task_context_t  context;         // 레지스터 저장 공간
    task_state_t    state;
    uint32_t        pid;             // 프로세스 ID
    int             priority;        // 우선순위 (nice 값, 낮을수록 높은 우선순위)
    struct task*    next;            // 링크드 리스트
} task_t;

#define MAX_TASKS 8
#define STACK_SIZE 4096  // 4KB 스택

static task_t tasks[MAX_TASKS];
static int num_tasks = 0;
static task_t* current_task = 0;
static task_t* idle_task = 0;
static int preemptive_enabled = 0;

// 외부 함수 선언
extern void context_switch(task_context_t* from, task_context_t* to);
extern void task_entry_wrapper(void);
extern void enter_usermode(uint32_t entry_point, uint32_t stack_pointer);

// kmalloc 선언 (kernel.c에서)
extern void* kmalloc(uint32_t size);

// 간단한 콘솔 출력 함수들 (kernel.c에서)
extern void console_write(const char* s, uint8_t color);
extern void console_write_dec(uint32_t value, uint8_t color);
extern void console_putc(char c, uint8_t color);
extern uint8_t make_color(uint8_t fg, uint8_t bg);

// 태스크 생성 (커널 모드)
task_t* task_create(const char* name, void (*entry)(void)) {
    if (num_tasks >= MAX_TASKS) return 0;
    
    task_t* task = &tasks[num_tasks];
    task->name = name;
    task->entry = entry;
    task->state = TASK_STATE_READY;
    task->pid = num_tasks;
    task->priority = 0;  // 기본 우선순위
    task->next = 0;
    
    // 스택 할당 (4KB)
    void* stack = kmalloc(STACK_SIZE);
    if (!stack) return 0;
    
    task->stack_top = (uint32_t)stack + STACK_SIZE;
    task->stack_size = STACK_SIZE;
    
    // 스택 초기화: 태스크가 처음 실행될 때 pop할 값들 설정
    // 스택은 높은 주소에서 낮은 주소로 자라므로, stack_top에서 시작
    uint32_t* stack_ptr = (uint32_t*)(task->stack_top);
    
    // 스택에 push할 값들 (역순으로)
    *--stack_ptr = 0x202;  // EFLAGS (IF=1, IOPL=0)
    *--stack_ptr = (uint32_t)task_entry_wrapper;  // EIP (태스크 진입점)
    *--stack_ptr = 0;  // EAX
    *--stack_ptr = 0;  // ECX
    *--stack_ptr = 0;  // EDX
    *--stack_ptr = 0;  // EBX
    *--stack_ptr = 0;  // ESP (나중에 실제 스택 포인터로 설정)
    *--stack_ptr = 0;  // EBP
    *--stack_ptr = 0;  // ESI
    *--stack_ptr = 0;  // EDI
    
    // 컨텍스트의 ESP를 초기화된 스택 포인터로 설정
    task->context.esp = (uint32_t)stack_ptr;
    task->context.eip = (uint32_t)task_entry_wrapper;
    task->context.eflags = 0x202;
    
    // 링크드 리스트 연결
    if (num_tasks > 0) {
        tasks[num_tasks - 1].next = task;
    }
    
    ++num_tasks;
    return task;
}

// 유저 모드 진입 래퍼 (전역 함수)
static void usermode_entry_wrapper(void) {
    if (current_task && current_task->entry) {
        // 유저 모드로 전환
        enter_usermode((uint32_t)current_task->entry, current_task->stack_top);
    }
}

// 유저 모드 태스크 생성
task_t* task_create_usermode(const char* name, void (*entry)(void)) {
    task_t* task = task_create(name, entry);
    if (!task) return 0;
    
    // 컨텍스트의 EIP를 유저 모드 진입 래퍼로 변경
    task->context.eip = (uint32_t)usermode_entry_wrapper;
    
    // 스택도 다시 설정
    uint32_t* stack_ptr = (uint32_t*)(task->stack_top);
    *--stack_ptr = 0x202;  // EFLAGS
    *--stack_ptr = (uint32_t)usermode_entry_wrapper;  // EIP
    *--stack_ptr = 0;  // EAX
    *--stack_ptr = 0;  // ECX
    *--stack_ptr = 0;  // EDX
    *--stack_ptr = 0;  // EBX
    *--stack_ptr = 0;  // ESP
    *--stack_ptr = 0;  // EBP
    *--stack_ptr = 0;  // ESI
    *--stack_ptr = 0;  // EDI
    task->context.esp = (uint32_t)stack_ptr;
    
    return task;
}

// 태스크 진입점 래퍼 구현 (C에서 직접 호출)
static void task_entry_wrapper_impl(void) {
    if (current_task && current_task->entry) {
        current_task->entry();
    }
    // 태스크가 종료되면 상태를 DONE으로 변경
    if (current_task) {
        current_task->state = TASK_STATE_DONE;
    }
    // 무한 루프로 대기 (나중에 스케줄러가 처리)
    for (;;) {
        __asm__ volatile("hlt");
    }
}

// 어셈블리 래퍼가 호출할 함수
void task_entry_wrapper(void) {
    task_entry_wrapper_impl();
}

// 스케줄러: 다음 태스크 선택 (우선순위 기반)
static task_t* scheduler_next(void) {
    if (num_tasks == 0) return idle_task;
    
    task_t* start = current_task ? current_task->next : &tasks[0];
    if (!start) start = &tasks[0];
    
    task_t* best_task = 0;
    int best_priority = 1000;  // 높은 숫자 = 낮은 우선순위
    task_t* task = start;
    int attempts = 0;
    
    // READY 또는 RUNNING 상태인 태스크 중 우선순위가 가장 높은 것 찾기
    do {
        if ((task->state == TASK_STATE_READY || task->state == TASK_STATE_RUNNING) &&
            task->priority < best_priority) {
            best_task = task;
            best_priority = task->priority;
        }
        task = task->next ? task->next : &tasks[0];
        ++attempts;
    } while (attempts < num_tasks);
    
    if (best_task) return best_task;
    
    // 모든 태스크가 종료되었으면 idle_task 반환
    return idle_task;
}

// 태스크 우선순위 설정 (nice 값)
void task_set_priority(uint32_t pid, int nice) {
    for (int i = 0; i < num_tasks; ++i) {
        if (tasks[i].pid == pid) {
            // nice 값 범위: -20 ~ 19 (리눅스 표준)
            if (nice < -20) nice = -20;
            if (nice > 19) nice = 19;
            tasks[i].priority = nice + 20;  // 0~39로 변환 (낮을수록 높은 우선순위)
            return;
        }
    }
}

// 컨텍스트 스위칭 (타이머 인터럽트에서 호출)
void schedule(void) {
    if (!preemptive_enabled || num_tasks == 0) return;
    
    task_t* next = scheduler_next();
    if (!next || next == current_task) return;
    
    task_t* prev = current_task;
    current_task = next;
    
    if (prev) {
        prev->state = TASK_STATE_READY;
    }
    next->state = TASK_STATE_RUNNING;
    
    // 시그널 처리
    extern void signal_process(uint32_t pid);
    signal_process(next->pid);
    
    // 컨텍스트 스위칭 (어셈블리 함수 호출)
    if (prev) {
        context_switch(&prev->context, &next->context);
    } else {
        // 첫 번째 태스크 시작
        __asm__ volatile(
            "mov %0, %%esp\n"
            "popa\n"
            "pop %%gs\n"
            "pop %%fs\n"
            "pop %%es\n"
            "pop %%ds\n"
            "iret\n"
            :: "r"(next->context.esp)
        );
    }
}

// 선점형 멀티태스킹 초기화
void preemptive_init(void) {
    num_tasks = 0;
    current_task = 0;
    preemptive_enabled = 0;
    
    // Idle 태스크 생성 (모든 태스크가 종료되었을 때 실행)
    idle_task = task_create("idle", 0);
    if (idle_task) {
        idle_task->state = TASK_STATE_RUNNING;
    }
}

// 선점형 멀티태스킹 활성화
void preemptive_enable(void) {
    preemptive_enabled = 1;
}

// 선점형 멀티태스킹 비활성화
void preemptive_disable(void) {
    preemptive_enabled = 0;
}

// 현재 태스크 종료
void task_exit_current(void) {
    if (current_task) {
        current_task->state = TASK_STATE_DONE;
    }
}

// 현재 태스크를 SLEEPING 상태로 변경
void task_sleep_current(void) {
    if (current_task) {
        current_task->state = TASK_STATE_SLEEPING;
    }
}

// 현재 프로세스 ID 반환
uint32_t task_get_current_pid(void) {
    if (current_task) {
        return current_task->pid;
    }
    return 0;
}

// 현재 태스크 복제 (fork) - PID 반환
uint32_t task_fork_current_pid(void) {
    if (!current_task || num_tasks >= MAX_TASKS) {
        return (uint32_t)-1;
    }
    
    task_t* parent = current_task;
    task_t* child = &tasks[num_tasks];
    
    // 기본 정보 복사
    child->name = parent->name;
    child->entry = parent->entry;
    child->state = TASK_STATE_READY;
    child->pid = num_tasks;
    child->next = 0;
    
    // 스택 할당
    void* stack = kmalloc(STACK_SIZE);
    if (!stack) return (uint32_t)-1;
    
    child->stack_top = (uint32_t)stack + STACK_SIZE;
    child->stack_size = STACK_SIZE;
    
    // 부모의 스택 내용 복사
    uint32_t* parent_stack = (uint32_t*)parent->stack_top;
    uint32_t* child_stack = (uint32_t*)child->stack_top;
    uint32_t stack_bytes = STACK_SIZE;
    
    // 스택 복사 (역순으로)
    for (uint32_t i = 0; i < stack_bytes / 4; ++i) {
        child_stack[-i] = parent_stack[-i];
    }
    
    // 컨텍스트 복사
    child->context = parent->context;
    child->context.esp = child->stack_top - (parent->stack_top - parent->context.esp);
    
    // 링크드 리스트 연결
    if (num_tasks > 0) {
        tasks[num_tasks - 1].next = child;
    }
    
    ++num_tasks;
    return child->pid;
}

// 현재 태스크 정보 출력
void task_print_info(void) {
    uint8_t color = make_color(0x0F, 0x01);
    console_write("Tasks:\n", color);
    for (int i = 0; i < num_tasks; ++i) {
        task_t* t = &tasks[i];
        console_write("  [", color);
        console_write_dec(t->pid, color);
        console_write("] ", color);
        console_write(t->name, color);
        console_write(" - state: ", color);
        if (t->state == TASK_STATE_READY) console_write("READY", color);
        else if (t->state == TASK_STATE_RUNNING) console_write("RUNNING", color);
        else if (t->state == TASK_STATE_DONE) console_write("DONE", color);
        else console_write("UNKNOWN", color);
        console_putc('\n', color);
    }
}

// 데모 태스크 함수들
void preemptive_taskA(void) {
    uint8_t color = make_color(0x0E, 0x01);
    for (int i = 0; i < 20; ++i) {
        console_write("[TaskA #", color);
        console_write_dec(i, color);
        console_write("]\n", color);
        
        // 짧은 딜레이 (타이머 인터럽트가 발생하도록)
        for (volatile int j = 0; j < 100000; ++j);
    }
    console_write("TaskA finished!\n", color);
}

void preemptive_taskB(void) {
    uint8_t color = make_color(0x0B, 0x01);
    for (int i = 0; i < 20; ++i) {
        console_write("[TaskB #", color);
        console_write_dec(i, color);
        console_write("]\n", color);
        
        // 짧은 딜레이
        for (volatile int j = 0; j < 100000; ++j);
    }
    console_write("TaskB finished!\n", color);
}

// 선점형 멀티태스킹 데모
void run_preemptive_demo(void) {
    uint8_t color = make_color(0x0F, 0x01);
    
    console_write("\n=== Preemptive Multitasking Demo ===\n", color);
    console_write("Two tasks will run preemptively via timer interrupts.\n", color);
    console_write("Each task will run 20 iterations.\n\n", color);
    
    preemptive_init();
    
    task_create("TaskA", preemptive_taskA);
    task_create("TaskB", preemptive_taskB);
    
    preemptive_enable();
    
    // 첫 번째 태스크 시작
    current_task = scheduler_next();
    if (current_task) {
        current_task->state = TASK_STATE_RUNNING;
        // 첫 태스크는 직접 시작 (나중에 스케줄러가 전환)
        console_write("Starting preemptive multitasking...\n", color);
        console_write("(Tasks will switch automatically via timer interrupts)\n\n", color);
    }
}

// 유저 모드 데모 태스크
void usermode_demo_task(void) {
    // 유저 모드에서 시스템 콜 사용
    // 인라인 어셈블리로 int 0x80 호출
    const char* msg = "Hello from user mode!\n";
    uint32_t len = 21;
    
    __asm__ volatile(
        "int $0x80"
        :
        : "a"(1),      // SYS_WRITE
          "b"(1),      // fd (stdout)
          "c"((uint32_t)msg),  // buf
          "d"(len)     // len
    );
    
    // 종료
    __asm__ volatile(
        "int $0x80"
        :
        : "a"(3),      // SYS_EXIT
          "b"(0)       // status
    );
}

// 유저 모드 데모
void run_usermode_demo(void) {
    uint8_t color = make_color(0x0F, 0x01);
    
    console_write("\n=== User Mode Demo ===\n", color);
    console_write("Creating a task that runs in user mode (Ring 3).\n", color);
    console_write("The task will use system calls to print a message.\n\n", color);
    
    preemptive_init();
    
    // 유저 모드 태스크 생성
    task_t* task = task_create_usermode("UserTask", usermode_demo_task);
    if (task) {
        console_write("User mode task created. Starting...\n", color);
        preemptive_enable();
        
        // 첫 태스크 시작
        current_task = task;
        current_task->state = TASK_STATE_RUNNING;
    } else {
        console_write("Failed to create user mode task.\n", color);
    }
}

