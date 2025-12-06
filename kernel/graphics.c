// kernel/graphics.c - VBE 그래픽 모드 지원
// 프레임버퍼 그래픽

#include <stdint.h>

// VBE 정보 구조체
typedef struct {
    uint16_t x_resolution;
    uint16_t y_resolution;
    uint8_t  bits_per_pixel;
    uint32_t framebuffer_addr;
    int      initialized;
} vbe_info_t;

static vbe_info_t vbe_info = {0};

// VBE 초기화 (Multiboot에서 정보 가져오기)
void graphics_init(uint32_t multiboot_info_addr) {
    // Multiboot VBE 정보 파싱 (간단한 구현)
    // 실제로는 Multiboot 구조체에서 VBE 정보를 읽어야 함
    vbe_info.initialized = 0;
    
    // 기본 VGA 텍스트 모드 사용 중이므로 그래픽 모드는 나중에 구현
    // 지금은 플레이스홀더
}

// 픽셀 그리기
void graphics_put_pixel(uint16_t x, uint16_t y, uint32_t color) {
    if (!vbe_info.initialized) return;
    if (x >= vbe_info.x_resolution || y >= vbe_info.y_resolution) return;
    
    uint32_t offset = (y * vbe_info.x_resolution + x) * (vbe_info.bits_per_pixel / 8);
    uint32_t* fb = (uint32_t*)(vbe_info.framebuffer_addr + offset);
    *fb = color;
}

// 그래픽 모드 활성화 여부
int graphics_is_available(void) {
    return vbe_info.initialized;
}

