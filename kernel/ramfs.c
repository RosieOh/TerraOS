// kernel/ramfs.c - 확장된 RAM 디스크 파일 시스템
// 디렉토리 구조 및 파일 쓰기 지원

#include <stdint.h>

// 파일 권한
#define PERM_READ   (1 << 0)
#define PERM_WRITE  (1 << 1)
#define PERM_EXEC   (1 << 2)

typedef struct {
    const char* name;
    const char* data;
    uint32_t    size;
    int         is_dir;  // 디렉토리 여부
    int         writable; // 쓰기 가능 여부
    uint32_t    permissions; // 파일 권한 (rwx)
    uint32_t    owner;       // 소유자 ID
    uint32_t    group;       // 그룹 ID
} ramfs_file_t;

// 간단한 데모 파일들
static const char file_hello[] =
    "Hello from terraOS RAMFS!\n"
    "This is a simple in-memory file.\n";

static const char file_readme[] =
    "terraOS mini README\n"
    "- hobby OS kernel\n"
    "- text console, keyboard, basic shell\n"
    "- interrupts, timer, simple memory & RAMFS\n"
    "- ELF loader, user mode programs\n";

// hello.elf 바이너리 (ramfs_hello.c에서 생성됨)
extern const unsigned char hello_elf[];
extern const uint32_t hello_elf_size;

#define MAX_RAMFS_FILES 32
static ramfs_file_t ramfs_files[MAX_RAMFS_FILES];
static uint32_t ramfs_file_count = 0;

// 동적 파일 데이터 저장 (쓰기 가능한 파일용)
static char dynamic_file_data[MAX_RAMFS_FILES][4096];
static int dynamic_file_used[MAX_RAMFS_FILES];

// RAM 디스크 초기화
void ramfs_init(void) {
    ramfs_file_count = 0;
    for (int i = 0; i < MAX_RAMFS_FILES; ++i) {
        dynamic_file_used[i] = 0;
    }
    
    ramfs_files[ramfs_file_count].name = "hello.txt";
    ramfs_files[ramfs_file_count].data = file_hello;
    ramfs_files[ramfs_file_count].size = sizeof(file_hello) - 1;
    ramfs_files[ramfs_file_count].is_dir = 0;
    ramfs_files[ramfs_file_count].writable = 0;
    ramfs_files[ramfs_file_count].permissions = PERM_READ | PERM_WRITE;
    ramfs_files[ramfs_file_count].owner = 0;
    ramfs_files[ramfs_file_count].group = 0;
    ++ramfs_file_count;
    
    ramfs_files[ramfs_file_count].name = "README.txt";
    ramfs_files[ramfs_file_count].data = file_readme;
    ramfs_files[ramfs_file_count].size = sizeof(file_readme) - 1;
    ramfs_files[ramfs_file_count].is_dir = 0;
    ramfs_files[ramfs_file_count].writable = 0;
    ramfs_files[ramfs_file_count].permissions = PERM_READ | PERM_WRITE;
    ramfs_files[ramfs_file_count].owner = 0;
    ramfs_files[ramfs_file_count].group = 0;
    ++ramfs_file_count;
    
    ramfs_files[ramfs_file_count].name = "hello.elf";
    ramfs_files[ramfs_file_count].data = (const char*)hello_elf;
    ramfs_files[ramfs_file_count].size = hello_elf_size;
    ramfs_files[ramfs_file_count].is_dir = 0;
    ramfs_files[ramfs_file_count].writable = 0;
    ramfs_files[ramfs_file_count].permissions = PERM_READ | PERM_EXEC;
    ramfs_files[ramfs_file_count].owner = 0;
    ramfs_files[ramfs_file_count].group = 0;
    ++ramfs_file_count;
}

// 파일 생성
int ramfs_create(const char* name, int is_dir) {
    if (ramfs_file_count >= MAX_RAMFS_FILES) return 0;
    
    // 동적 데이터 할당
    int data_idx = -1;
    for (int i = 0; i < MAX_RAMFS_FILES; ++i) {
        if (!dynamic_file_used[i]) {
            data_idx = i;
            dynamic_file_used[i] = 1;
            break;
        }
    }
    if (data_idx < 0) return 0;
    
    ramfs_files[ramfs_file_count].name = name;
    ramfs_files[ramfs_file_count].data = dynamic_file_data[data_idx];
    ramfs_files[ramfs_file_count].size = 0;
    ramfs_files[ramfs_file_count].is_dir = is_dir;
    ramfs_files[ramfs_file_count].writable = 1;
    ramfs_files[ramfs_file_count].permissions = PERM_READ | PERM_WRITE | (is_dir ? 0 : 0);
    ramfs_files[ramfs_file_count].owner = 0;
    ramfs_files[ramfs_file_count].group = 0;
    ++ramfs_file_count;
    
    return 1;
}

// 파일 삭제
int ramfs_delete(const char* name) {
    for (uint32_t i = 0; i < ramfs_file_count; ++i) {
        const char* n = ramfs_files[i].name;
        const char* p = name;
        while (*n && *p && *n == *p) {
            ++n;
            ++p;
        }
        if (*n == '\0' && *p == '\0') {
            // 파일 제거 (간단히 이름을 빈 문자열로)
            ramfs_files[i].name = "";
            return 1;
        }
    }
    return 0;
}

// 파일 쓰기
int ramfs_write(const char* name, const char* data, uint32_t size) {
    for (uint32_t i = 0; i < ramfs_file_count; ++i) {
        const char* n = ramfs_files[i].name;
        const char* p = name;
        while (*n && *p && *n == *p) {
            ++n;
            ++p;
        }
        if (*n == '\0' && *p == '\0' && ramfs_files[i].writable) {
            if (size > 4095) size = 4095;
            for (uint32_t j = 0; j < size; ++j) {
                ((char*)ramfs_files[i].data)[j] = data[j];
            }
            ((char*)ramfs_files[i].data)[size] = '\0';
            ramfs_files[i].size = size;
            return 1;
        }
    }
    return 0;
}

uint32_t ramfs_list(ramfs_file_t** out_files) {
    if (out_files) {
        *out_files = ramfs_files;
    }
    return ramfs_file_count;
}

int ramfs_read(const char* name, const char** out_data, uint32_t* out_size) {
    if (!name) return 0;
    for (uint32_t i = 0; i < ramfs_file_count; ++i) {
        const char* n = ramfs_files[i].name;
        const char* p = name;
        while (*n && *p && *n == *p) {
            ++n;
            ++p;
        }
        if (*n == '\0' && *p == '\0') {
            if (out_data) *out_data = ramfs_files[i].data;
            if (out_size) *out_size = ramfs_files[i].size;
            return 1;
        }
    }
    return 0;
}


