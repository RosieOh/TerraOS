// kernel/smp.c - 멀티코어 지원 (SMP)
// CPU별 스케줄러 및 APIC 초기화

#include <stdint.h>

#define MAX_CPUS 4

// CPU 정보
typedef struct {
    int present;
    int apic_id;
    uint32_t* page_directory;
} cpu_info_t;

static cpu_info_t cpus[MAX_CPUS];
static int cpu_count = 1;  // 기본적으로 1개 CPU

// CPU 초기화
void smp_init(void) {
    cpu_count = 1;  // 기본적으로 단일 코어로 시작
    
    for (int i = 0; i < MAX_CPUS; ++i) {
        cpus[i].present = (i == 0) ? 1 : 0;
        cpus[i].apic_id = i;
        cpus[i].page_directory = 0;
    }
    
    // APIC 초기화는 나중에 구현
    // 지금은 단일 코어만 지원
}

// CPU 개수 반환
int smp_get_cpu_count(void) {
    return cpu_count;
}

// 현재 CPU ID
int smp_get_current_cpu(void) {
    return 0;  // 항상 첫 번째 CPU
}

// CPU별 스케줄러 (나중에 확장)
void smp_schedule(int cpu_id) {
    (void)cpu_id;
    // CPU별 태스크 큐는 나중에 구현
}

