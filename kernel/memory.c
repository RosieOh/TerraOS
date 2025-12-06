// kernel/memory.c - 고급 메모리 관리
// 페이지 프레임 할당 및 가상 메모리 매핑

#include <stdint.h>

#define PAGE_SIZE 4096
#define FRAME_SIZE PAGE_SIZE

// 페이지 프레임 비트맵 (간단한 구현)
#define MAX_FRAMES 1024  // 4MB / 4KB = 1024 프레임
static uint8_t frame_bitmap[MAX_FRAMES / 8];
static uint32_t total_frames = 0;
static uint32_t used_frames = 0;

// 프레임 비트맵 초기화
void frame_init(uint32_t total_memory_kb) {
    // 사용 가능한 프레임 수 계산
    total_frames = (total_memory_kb * 1024) / FRAME_SIZE;
    if (total_frames > MAX_FRAMES) {
        total_frames = MAX_FRAMES;
    }
    
    // 커널이 사용하는 메모리는 이미 할당된 것으로 표시
    extern uint32_t _kernel_end;
    uint32_t kernel_end = (uint32_t)&_kernel_end;
    uint32_t kernel_frames = (kernel_end + FRAME_SIZE - 1) / FRAME_SIZE;
    
    // 비트맵 초기화
    for (uint32_t i = 0; i < MAX_FRAMES / 8; ++i) {
        frame_bitmap[i] = 0;
    }
    
    // 커널 영역 마킹
    for (uint32_t i = 0; i < kernel_frames && i < total_frames; ++i) {
        uint32_t byte_idx = i / 8;
        uint32_t bit_idx = i % 8;
        frame_bitmap[byte_idx] |= (1 << bit_idx);
        ++used_frames;
    }
}

// 프레임 할당
uint32_t frame_alloc(void) {
    for (uint32_t i = 0; i < total_frames; ++i) {
        uint32_t byte_idx = i / 8;
        uint32_t bit_idx = i % 8;
        
        if (!(frame_bitmap[byte_idx] & (1 << bit_idx))) {
            // 프레임 할당
            frame_bitmap[byte_idx] |= (1 << bit_idx);
            ++used_frames;
            return i * FRAME_SIZE;
        }
    }
    return 0;  // 할당 실패
}

// 프레임 해제
void frame_free(uint32_t frame_addr) {
    uint32_t frame_num = frame_addr / FRAME_SIZE;
    if (frame_num >= total_frames) return;
    
    uint32_t byte_idx = frame_num / 8;
    uint32_t bit_idx = frame_num % 8;
    
    if (frame_bitmap[byte_idx] & (1 << bit_idx)) {
        frame_bitmap[byte_idx] &= ~(1 << bit_idx);
        --used_frames;
    }
}

// 페이지 테이블 엔트리 찾기/생성
static uint32_t* get_page_table_entry(uint32_t vaddr) {
    // 현재 페이지 디렉터리 가져오기
    uint32_t* page_dir = 0;
    __asm__ volatile("mov %%cr3, %0" : "=r"(page_dir));
    
    uint32_t dir_idx = (vaddr >> 22) & 0x3FF;
    uint32_t table_idx = (vaddr >> 12) & 0x3FF;
    
    // 페이지 디렉터리 엔트리 확인
    if (!(page_dir[dir_idx] & 0x1)) {
        // 페이지 테이블이 없으면 생성
        uint32_t frame = frame_alloc();
        if (!frame) return 0;
        
        uint32_t* new_table = (uint32_t*)frame;
        // 테이블 초기화
        for (int i = 0; i < 1024; ++i) {
            new_table[i] = 0;
        }
        
        // 디렉터리에 연결
        page_dir[dir_idx] = frame | 0x3;  // Present | RW
    }
    
    uint32_t* page_table = (uint32_t*)(page_dir[dir_idx] & ~0xFFF);
    return &page_table[table_idx];
}

// 가상 주소를 물리 주소에 매핑
int page_map(uint32_t vaddr, uint32_t paddr, uint32_t flags) {
    uint32_t* entry = get_page_table_entry(vaddr);
    if (!entry) return -1;
    
    *entry = paddr | flags | 0x1;  // Present
    return 0;
}

// 페이지 매핑 해제
void page_unmap(uint32_t vaddr) {
    uint32_t* entry = get_page_table_entry(vaddr);
    if (entry && (*entry & 0x1)) {
        uint32_t frame = *entry & ~0xFFF;
        *entry = 0;
        frame_free(frame);
    }
}

// 메모리 정보 반환
void memory_get_info(uint32_t* total, uint32_t* used) {
    if (total) *total = total_frames * FRAME_SIZE;
    if (used) *used = used_frames * FRAME_SIZE;
}

