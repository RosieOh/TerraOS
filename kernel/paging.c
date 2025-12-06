// kernel/paging.c - 가장 기본적인 페이지 단위 메모리 관리 (4MiB identity map)
// 향후 Mint Linux 스타일로 확장 가능한 첫 단계: 페이징 활성화만 담당

#include <stdint.h>

#define PAGE_SIZE     4096
#define NUM_ENTRIES   1024

// 페이지 디렉터리와 첫 번째 페이지 테이블
// 4KiB 정렬이 필요하므로 aligned 속성을 붙인다.
static uint32_t __attribute__((aligned(4096))) page_directory[NUM_ENTRIES];
static uint32_t __attribute__((aligned(4096))) first_page_table[NUM_ENTRIES];

static inline void load_page_directory(uint32_t* pd) {
    __asm__ volatile("mov %0, %%cr3" :: "r"(pd));
}

static inline void enable_paging(void) {
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000u; // PG 비트
    __asm__ volatile("mov %0, %%cr0" :: "r"(cr0));
}

void paging_init(void) {
    // 페이지 디렉터리/테이블 초기화 (모두 0으로)
    for (int i = 0; i < NUM_ENTRIES; ++i) {
        page_directory[i] = 0;
        first_page_table[i] = 0;
    }

    // 0 ~ 4MiB 영역을 identity mapping
    for (int i = 0; i < NUM_ENTRIES; ++i) {
        uint32_t addr = (uint32_t)i * PAGE_SIZE;
        // Present(1) | RW(2) | Supervisor(0)
        first_page_table[i] = addr | 0x3;
    }

    // 페이지 디렉터리의 첫 엔트리에 첫 페이지 테이블을 연결
    page_directory[0] = (uint32_t)first_page_table | 0x3;

    load_page_directory(page_directory);
    enable_paging();
}


