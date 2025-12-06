// kernel/heap.c - 개선된 힙 할당자
// free 지원 및 메모리 관리 개선

#include <stdint.h>

#define HEAP_MAGIC 0xDEADBEEF
#define MIN_BLOCK_SIZE 16

// 힙 블록 헤더
typedef struct heap_block {
    uint32_t magic;
    uint32_t size;
    int free;
    struct heap_block* next;
    struct heap_block* prev;
} heap_block_t;

static heap_block_t* heap_start = 0;
static heap_block_t* heap_end = 0;
static uint32_t heap_base = 0;
static uint32_t heap_limit = 0;

// 힙 초기화
void heap_init(uint32_t base, uint32_t size) {
    heap_base = base;
    heap_limit = base + size;
    heap_start = (heap_block_t*)base;
    heap_end = (heap_block_t*)(base + size - sizeof(heap_block_t));
    
    // 초기 블록 설정
    heap_start->magic = HEAP_MAGIC;
    heap_start->size = size - sizeof(heap_block_t);
    heap_start->free = 1;
    heap_start->next = 0;
    heap_start->prev = 0;
}

// 블록 분할
static void heap_split_block(heap_block_t* block, uint32_t size) {
    if (block->size < size + sizeof(heap_block_t) + MIN_BLOCK_SIZE) {
        return;  // 분할할 공간이 부족
    }
    
    heap_block_t* new_block = (heap_block_t*)((char*)block + sizeof(heap_block_t) + size);
    new_block->magic = HEAP_MAGIC;
    new_block->size = block->size - size - sizeof(heap_block_t);
    new_block->free = 1;
    new_block->next = block->next;
    new_block->prev = block;
    
    if (block->next) {
        block->next->prev = new_block;
    }
    
    block->size = size;
    block->next = new_block;
}

// 인접한 빈 블록 병합
static void heap_merge_free(heap_block_t* block) {
    // 다음 블록과 병합
    if (block->next && block->next->free &&
        (char*)block + sizeof(heap_block_t) + block->size == (char*)block->next) {
        block->size += sizeof(heap_block_t) + block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }
    
    // 이전 블록과 병합
    if (block->prev && block->prev->free &&
        (char*)block->prev + sizeof(heap_block_t) + block->prev->size == (char*)block) {
        block->prev->size += sizeof(heap_block_t) + block->size;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }
}

// 메모리 할당
void* heap_alloc(uint32_t size) {
    if (size == 0) return 0;
    
    // 8바이트 정렬
    size = (size + 7) & ~7U;
    if (size < MIN_BLOCK_SIZE) size = MIN_BLOCK_SIZE;
    
    // First-fit 알고리즘
    heap_block_t* block = heap_start;
    while (block) {
        if (block->magic != HEAP_MAGIC) {
            return 0;  // 힙 손상
        }
        
        if (block->free && block->size >= size) {
            // 블록 분할
            heap_split_block(block, size);
            
            block->free = 0;
            return (void*)((char*)block + sizeof(heap_block_t));
        }
        
        block = block->next;
    }
    
    return 0;  // 메모리 부족
}

// 메모리 해제
void heap_free(void* ptr) {
    if (!ptr) return;
    
    heap_block_t* block = (heap_block_t*)((char*)ptr - sizeof(heap_block_t));
    
    if (block->magic != HEAP_MAGIC) {
        return;  // 잘못된 포인터
    }
    
    if (block->free) {
        return;  // 이미 해제됨
    }
    
    block->free = 1;
    heap_merge_free(block);
}

// 힙 정보
void heap_get_info(uint32_t* total, uint32_t* used, uint32_t* free) {
    uint32_t total_size = 0;
    uint32_t used_size = 0;
    uint32_t free_size = 0;
    
    heap_block_t* block = heap_start;
    while (block) {
        if (block->magic != HEAP_MAGIC) break;
        
        total_size += block->size + sizeof(heap_block_t);
        if (block->free) {
            free_size += block->size;
        } else {
            used_size += block->size;
        }
        
        block = block->next;
    }
    
    if (total) *total = total_size;
    if (used) *used = used_size;
    if (free) *free = free_size;
}

