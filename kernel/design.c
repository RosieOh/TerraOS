// kernel/design.c - terraOS 디자인 시스템
// 현대적이고 보기 좋은 UI/UX

#include <stdint.h>

// 외부 함수들
extern void console_write(const char* s, uint8_t color);
extern void console_putc(char c, uint8_t color);
extern void console_write_dec(uint32_t value, uint8_t color);
extern uint8_t make_color(uint8_t fg, uint8_t bg);
extern void clear_screen(uint8_t color);

// 색상 팔레트
#define COLOR_BLACK     0x00
#define COLOR_BLUE      0x01
#define COLOR_GREEN     0x02
#define COLOR_CYAN      0x03
#define COLOR_RED       0x04
#define COLOR_MAGENTA   0x05
#define COLOR_BROWN     0x06
#define COLOR_LIGHT_GRAY 0x07
#define COLOR_DARK_GRAY 0x08
#define COLOR_LIGHT_BLUE 0x09
#define COLOR_LIGHT_GREEN 0x0A
#define COLOR_LIGHT_CYAN 0x0B
#define COLOR_LIGHT_RED 0x0C
#define COLOR_LIGHT_MAGENTA 0x0D
#define COLOR_YELLOW    0x0E
#define COLOR_WHITE     0x0F

// 테마 색상
#define THEME_BG        COLOR_BLACK
#define THEME_FG        COLOR_LIGHT_GREEN
#define THEME_ACCENT    COLOR_LIGHT_CYAN
#define THEME_SUCCESS   COLOR_LIGHT_GREEN
#define THEME_WARNING   COLOR_YELLOW
#define THEME_ERROR     COLOR_LIGHT_RED
#define THEME_INFO      COLOR_LIGHT_BLUE

// 로고 출력
void design_print_logo(void) {
    uint8_t accent = make_color(THEME_ACCENT, THEME_BG);
    uint8_t fg = make_color(THEME_FG, THEME_BG);
    
    console_write("\n", accent);
    console_write("  _______ _____ _____ ____   ____  \n", accent);
    console_write(" |__   __|_   _|  __ \\|  _ \\ / __ \\ \n", accent);
    console_write("    | |    | | | |__) | |_) | |  | |\n", accent);
    console_write("    | |    | | |  _  /|  _ <| |  | |\n", accent);
    console_write("    | |   _| |_| | \\ \\| |_) | |__| |\n", accent);
    console_write("    |_|  |_____|_|  \\_\\____/ \\____/ \n", accent);
    console_write("\n", accent);
    console_write("     Operating System v1.0\n", fg);
    console_write("     Mint Linux Inspired\n", fg);
    console_write("\n", accent);
}

// 구분선 출력
void design_print_separator(char c) {
    uint8_t color = make_color(THEME_ACCENT, THEME_BG);
    for (int i = 0; i < 80; ++i) {
        console_putc(c, color);
    }
    console_putc('\n', color);
}

// 성공 메시지
void design_success(const char* message) {
    uint8_t color = make_color(THEME_SUCCESS, THEME_BG);
    console_write("[✓] ", color);
    console_write(message, color);
    console_putc('\n', color);
}

// 경고 메시지
void design_warning(const char* message) {
    uint8_t color = make_color(THEME_WARNING, THEME_BG);
    console_write("[!] ", color);
    console_write(message, color);
    console_putc('\n', color);
}

// 에러 메시지
void design_error(const char* message) {
    uint8_t color = make_color(THEME_ERROR, THEME_BG);
    console_write("[✗] ", color);
    console_write(message, color);
    console_putc('\n', color);
}

// 정보 메시지
void design_info(const char* message) {
    uint8_t color = make_color(THEME_INFO, THEME_BG);
    console_write("[i] ", color);
    console_write(message, color);
    console_putc('\n', color);
}

// 박스 출력 (테이블/리스트용)
void design_print_box(const char* title) {
    uint8_t accent = make_color(THEME_ACCENT, THEME_BG);
    uint8_t fg = make_color(THEME_FG, THEME_BG);
    
    console_write("┌─ ", accent);
    console_write(title, fg);
    int len = 0;
    while (title[len]) ++len;
    for (int i = len + 3; i < 79; ++i) {
        console_putc('─', accent);
    }
    console_write("┐\n", accent);
}

void design_close_box(void) {
    uint8_t accent = make_color(THEME_ACCENT, THEME_BG);
    console_putc('└', accent);
    for (int i = 0; i < 78; ++i) {
        console_putc('─', accent);
    }
    console_write("┘\n", accent);
}

// 진행 바 (간단한 버전)
void design_progress_bar(uint32_t current, uint32_t total, const char* label) {
    uint8_t fg = make_color(THEME_FG, THEME_BG);
    uint8_t accent = make_color(THEME_ACCENT, THEME_BG);
    
    if (label) {
        console_write(label, fg);
        console_write(": ", fg);
    }
    
    uint32_t percent = (total > 0) ? (current * 100) / total : 0;
    if (percent > 100) percent = 100;
    
    console_putc('[', accent);
    int bar_width = 20;
    int filled = (percent * bar_width) / 100;
    
    for (int i = 0; i < bar_width; ++i) {
        if (i < filled) {
            console_putc('=', accent);
        } else {
            console_putc(' ', fg);
        }
    }
    
    console_putc(']', accent);
    console_write(" ", fg);
    console_write_dec(percent, fg);
    console_write("%\n", fg);
}

// 부팅 화면
void design_boot_screen(void) {
    clear_screen(make_color(THEME_BG, THEME_BG));
    
    design_print_logo();
    
    uint8_t info = make_color(THEME_INFO, THEME_BG);
    console_write("Initializing system components...\n\n", info);
}

// 프롬프트 스타일
void design_print_prompt(void) {
    uint8_t accent = make_color(THEME_ACCENT, THEME_BG);
    uint8_t fg = make_color(THEME_FG, THEME_BG);
    
    // 현재 디렉토리 가져오기
    extern const char* path_getcwd(void);
    const char* cwd = "/";  // 기본값
    const char* cwd_actual = path_getcwd();
    if (cwd_actual) cwd = cwd_actual;
    
    console_write("┌─[", accent);
    console_write("terraOS", fg);
    console_write("]─[", accent);
    console_write(cwd, fg);
    console_write("]\n", accent);
    console_write("└─$ ", accent);
}

