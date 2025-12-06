// kernel/vfs.c - 가상 파일 시스템 (VFS)
// 파일 시스템 추상화 및 마운트 포인트

#include <stdint.h>

#define MAX_MOUNTS 8
#define MAX_PATH_LEN 256

// 파일 시스템 타입
typedef enum {
    FS_TYPE_RAMFS,
    FS_TYPE_NONE
} fs_type_t;

// 마운트 포인트
typedef struct {
    char mount_point[MAX_PATH_LEN];
    fs_type_t fs_type;
    void* fs_data;  // 파일 시스템별 데이터
    int valid;
} mount_t;

static mount_t mounts[MAX_MOUNTS];
static int mount_count = 0;

// 루트 파일 시스템 초기화
void vfs_init(void) {
    mount_count = 0;
    for (int i = 0; i < MAX_MOUNTS; ++i) {
        mounts[i].valid = 0;
    }
    
    // 루트를 RAMFS로 마운트
    mounts[0].mount_point[0] = '/';
    mounts[0].mount_point[1] = '\0';
    mounts[0].fs_type = FS_TYPE_RAMFS;
    mounts[0].fs_data = 0;  // RAMFS는 전역이므로
    mounts[0].valid = 1;
    mount_count = 1;
}

// 경로에서 마운트 포인트 찾기
static mount_t* vfs_find_mount(const char* path) {
    if (!path || path[0] != '/') return 0;
    
    // 루트는 항상 첫 번째 마운트
    if (path[1] == '\0' || path[1] == '/') {
        return &mounts[0];
    }
    
    // 나중에 다른 마운트 포인트 지원
    return &mounts[0];
}

// VFS를 통한 파일 읽기
int vfs_read(const char* path, const char** out_data, uint32_t* out_size) {
    mount_t* mount = vfs_find_mount(path);
    if (!mount || !mount->valid) return 0;
    
    if (mount->fs_type == FS_TYPE_RAMFS) {
        // RAMFS는 파일명만 사용
        const char* filename = path;
        while (*filename == '/') ++filename;
        if (*filename == '\0') filename = ".";
        
        extern int ramfs_read(const char* name, const char** out_data, uint32_t* out_size);
        return ramfs_read(filename, out_data, out_size);
    }
    
    return 0;
}

// VFS를 통한 파일 목록
uint32_t vfs_list(const char* path, void** out_files) {
    mount_t* mount = vfs_find_mount(path);
    if (!mount || !mount->valid) return 0;
    
    if (mount->fs_type == FS_TYPE_RAMFS) {
        extern uint32_t ramfs_list(void** out_files);
        return ramfs_list(out_files);
    }
    
    return 0;
}

