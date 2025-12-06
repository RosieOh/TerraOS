// kernel/signal.c - 시그널 처리 시스템
// 프로세스 간 시그널 전송 및 핸들러

#include <stdint.h>

#define MAX_SIGNALS 32
#define SIGKILL 9
#define SIGTERM 15
#define SIGUSR1 10
#define SIGUSR2 12

// 시그널 핸들러 타입
typedef void (*signal_handler_t)(int);

// 프로세스별 시그널 정보
typedef struct {
    signal_handler_t handlers[MAX_SIGNALS];
    uint32_t pending_signals;
    uint32_t blocked_signals;
} signal_info_t;

// 태스크별 시그널 정보 저장 (간단히 전역 배열)
#define MAX_PROCESSES 8
static signal_info_t process_signals[MAX_PROCESSES];

// 시그널 전송
int signal_send(uint32_t pid, int sig) {
    if (pid >= MAX_PROCESSES || sig >= MAX_SIGNALS) return -1;
    
    process_signals[pid].pending_signals |= (1U << sig);
    return 0;
}

// 시그널 핸들러 설정
int signal_set_handler(uint32_t pid, int sig, signal_handler_t handler) {
    if (pid >= MAX_PROCESSES || sig >= MAX_SIGNALS) return -1;
    
    process_signals[pid].handlers[sig] = handler;
    return 0;
}

// 시그널 처리 (프로세스가 실행될 때 호출)
void signal_process(uint32_t pid) {
    if (pid >= MAX_PROCESSES) return;
    
    signal_info_t* info = &process_signals[pid];
    uint32_t pending = info->pending_signals & ~info->blocked_signals;
    
    for (int sig = 0; sig < MAX_SIGNALS && pending; ++sig) {
        if (pending & (1U << sig)) {
            pending &= ~(1U << sig);
            info->pending_signals &= ~(1U << sig);
            
            if (info->handlers[sig]) {
                info->handlers[sig](sig);
            } else if (sig == SIGKILL || sig == SIGTERM) {
                // 기본 동작: 프로세스 종료
                extern void task_exit_current(void);
                task_exit_current();
            }
        }
    }
}

// 시그널 초기화
void signal_init(void) {
    for (int i = 0; i < MAX_PROCESSES; ++i) {
        process_signals[i].pending_signals = 0;
        process_signals[i].blocked_signals = 0;
        for (int j = 0; j < MAX_SIGNALS; ++j) {
            process_signals[i].handlers[j] = 0;
        }
    }
}

