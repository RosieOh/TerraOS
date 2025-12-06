// kernel/gdt.c - GDT (Global Descriptor Table) 설정
// 유저 모드 세그먼트 추가

#include <stdint.h>

// GDT 엔트리 구조체
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

// GDT 포인터 구조체
struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct gdt_entry gdt[6];
static struct gdt_ptr   gdtp;

// GDT 엔트리 설정
static void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;
    
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    
    gdt[num].access = access;
}

// GDT 초기화
void gdt_init(void) {
    gdtp.limit = (sizeof(struct gdt_entry) * 6) - 1;
    gdtp.base = (uint32_t)&gdt;
    
    // NULL 세그먼트 (0)
    gdt_set_gate(0, 0, 0, 0, 0);
    
    // 커널 코드 세그먼트 (0x08) - Ring 0
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    
    // 커널 데이터 세그먼트 (0x10) - Ring 0
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    
    // 유저 코드 세그먼트 (0x18) - Ring 3
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
    
    // 유저 데이터 세그먼트 (0x20) - Ring 3
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);
    
    // TSS 세그먼트 (0x28) - 나중에 사용
    gdt_set_gate(5, 0, 0, 0, 0);
    
    // GDT 로드
    __asm__ volatile("lgdt (%0)" :: "r"(&gdtp));
    
    // 세그먼트 레지스터 리로드
    __asm__ volatile(
        "mov $0x10, %ax\n"
        "mov %ax, %ds\n"
        "mov %ax, %es\n"
        "mov %ax, %fs\n"
        "mov %ax, %gs\n"
        "mov %ax, %ss\n"
        "ljmp $0x08, $1f\n"
        "1:\n"
    );
}

