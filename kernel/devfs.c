// kernel/devfs.c - 디바이스 파일 시스템
// /dev 디렉토리 및 디바이스 파일 지원

#include <stdint.h>

#define MAX_DEVICES 16
#define DEV_NAME_LEN 32

// 디바이스 타입
typedef enum {
    DEV_TYPE_CHAR,
    DEV_TYPE_BLOCK,
    DEV_TYPE_NULL,
    DEV_TYPE_ZERO,
    DEV_TYPE_CONSOLE
} dev_type_t;

// 디바이스 구조체
typedef struct {
    char name[DEV_NAME_LEN];
    dev_type_t type;
    uint32_t major;
    uint32_t minor;
    int valid;
    
    // 디바이스 읽기/쓰기 함수 포인터
    uint32_t (*read)(void* buffer, uint32_t size);
    uint32_t (*write)(const void* buffer, uint32_t size);
} device_t;

static device_t devices[MAX_DEVICES];
static int device_count = 0;

// /dev/null 읽기 (항상 EOF)
static uint32_t dev_null_read(void* buffer, uint32_t size) {
    (void)buffer;
    (void)size;
    return 0;  // EOF
}

// /dev/null 쓰기 (항상 성공)
static uint32_t dev_null_write(const void* buffer, uint32_t size) {
    (void)buffer;
    return size;  // 모든 데이터 소비
}

// /dev/zero 읽기 (항상 0)
static uint32_t dev_zero_read(void* buffer, uint32_t size) {
    char* buf = (char*)buffer;
    for (uint32_t i = 0; i < size; ++i) {
        buf[i] = 0;
    }
    return size;
}

// /dev/zero 쓰기 (항상 성공)
static uint32_t dev_zero_write(const void* buffer, uint32_t size) {
    (void)buffer;
    return size;
}

// /dev/console 읽기 (키보드)
static uint32_t dev_console_read(void* buffer, uint32_t size) {
    extern char keyboard_read_char(void);
    char* buf = (char*)buffer;
    uint32_t i = 0;
    while (i < size - 1) {
        char c = keyboard_read_char();
        if (c == '\n') {
            buf[i++] = '\n';
            break;
        }
        buf[i++] = c;
    }
    buf[i] = '\0';
    return i;
}

// /dev/console 쓰기 (화면)
static uint32_t dev_console_write(const void* buffer, uint32_t size) {
    extern void console_putc(char c, uint8_t color);
    extern uint8_t make_color(uint8_t fg, uint8_t bg);
    const char* buf = (const char*)buffer;
    uint8_t color = make_color(0x0F, 0x01);
    for (uint32_t i = 0; i < size; ++i) {
        console_putc(buf[i], color);
    }
    return size;
}

// 디바이스 등록
int devfs_register(const char* name, dev_type_t type, uint32_t major, uint32_t minor,
                   uint32_t (*read_func)(void*, uint32_t),
                   uint32_t (*write_func)(const void*, uint32_t)) {
    if (device_count >= MAX_DEVICES) return -1;
    
    int i = 0;
    while (name[i] && i < DEV_NAME_LEN - 1) {
        devices[device_count].name[i] = name[i];
        ++i;
    }
    devices[device_count].name[i] = '\0';
    devices[device_count].type = type;
    devices[device_count].major = major;
    devices[device_count].minor = minor;
    devices[device_count].read = read_func;
    devices[device_count].write = write_func;
    devices[device_count].valid = 1;
    ++device_count;
    
    return 0;
}

// 디바이스 찾기
device_t* devfs_find(const char* name) {
    for (int i = 0; i < device_count; ++i) {
        if (devices[i].valid) {
            int j = 0;
            while (devices[i].name[j] && name[j] && 
                   devices[i].name[j] == name[j]) {
                ++j;
            }
            if (devices[i].name[j] == '\0' && name[j] == '\0') {
                return &devices[i];
            }
        }
    }
    return 0;
}

// 디바이스 읽기
uint32_t devfs_read(const char* name, void* buffer, uint32_t size) {
    device_t* dev = devfs_find(name);
    if (!dev || !dev->read) return (uint32_t)-1;
    return dev->read(buffer, size);
}

// 디바이스 쓰기
uint32_t devfs_write(const char* name, const void* buffer, uint32_t size) {
    device_t* dev = devfs_find(name);
    if (!dev || !dev->write) return (uint32_t)-1;
    return dev->write(buffer, size);
}

// 디바이스 목록
int devfs_list(device_t** out_devices) {
    if (out_devices) {
        *out_devices = devices;
    }
    return device_count;
}

// 디바이스 목록 출력 (쉘 명령용)
void devfs_list_devices(void) {
    extern void console_write(const char* s, uint8_t color);
    extern void console_write_dec(uint32_t value, uint8_t color);
    extern void console_putc(char c, uint8_t color);
    extern uint8_t make_color(uint8_t fg, uint8_t bg);
    
    uint8_t color = make_color(0x0F, 0x00);
    console_write("Registered devices (/dev):\n", color);
    
    for (int i = 0; i < device_count; ++i) {
        if (devices[i].valid) {
            console_write("  /dev/", color);
            console_write(devices[i].name, color);
            console_write(" (major: ", color);
            console_write_dec(devices[i].major, color);
            console_write(", minor: ", color);
            console_write_dec(devices[i].minor, color);
            console_write(")\n", color);
        }
    }
}

// 디바이스 파일 시스템 초기화
void devfs_init(void) {
    device_count = 0;
    for (int i = 0; i < MAX_DEVICES; ++i) {
        devices[i].valid = 0;
    }
    
    // 기본 디바이스 등록
    devfs_register("null", DEV_TYPE_NULL, 1, 3, dev_null_read, dev_null_write);
    devfs_register("zero", DEV_TYPE_ZERO, 1, 5, dev_zero_read, dev_zero_write);
    devfs_register("console", DEV_TYPE_CONSOLE, 4, 0, dev_console_read, dev_console_write);
}

