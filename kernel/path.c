// kernel/path.c - 경로 처리
// 절대/상대 경로 파싱 및 정규화

#include <stdint.h>

#define MAX_PATH_LEN 256

static char current_directory[MAX_PATH_LEN] = "/";

// 현재 작업 디렉토리 가져오기
const char* path_getcwd(void) {
    return current_directory;
}

// 디렉토리 변경
int path_chdir(const char* path) {
    if (!path) return -1;
    
    // 간단한 구현: 절대 경로만 지원
    if (path[0] == '/') {
        int i = 0;
        while (path[i] && i < MAX_PATH_LEN - 1) {
            current_directory[i] = path[i];
            ++i;
        }
        current_directory[i] = '\0';
        return 0;
    }
    
    // 상대 경로는 나중에 구현
    return -1;
}

// 경로 정규화 (.., . 처리)
void path_normalize(char* path) {
    if (!path || path[0] != '/') return;
    
    // 간단한 구현: ..와 . 제거
    char result[MAX_PATH_LEN];
    result[0] = '/';
    int result_pos = 1;
    int path_pos = 1;
    
    while (path[path_pos]) {
        if (path[path_pos] == '.' && path[path_pos + 1] == '.' && 
            (path[path_pos + 2] == '/' || path[path_pos + 2] == '\0')) {
            // .. 처리: 상위 디렉토리로
            if (result_pos > 1) {
                --result_pos;
                while (result_pos > 1 && result[result_pos] != '/') {
                    --result_pos;
                }
            }
            path_pos += 2;
            if (path[path_pos] == '/') ++path_pos;
        } else if (path[path_pos] == '.' && 
                   (path[path_pos + 1] == '/' || path[path_pos + 1] == '\0')) {
            // . 처리: 현재 디렉토리 (무시)
            path_pos += 1;
            if (path[path_pos] == '/') ++path_pos;
        } else {
            // 일반 경로 복사
            while (path[path_pos] && path[path_pos] != '/') {
                if (result_pos < MAX_PATH_LEN - 1) {
                    result[result_pos++] = path[path_pos++];
                } else {
                    ++path_pos;
                }
            }
            if (path[path_pos] == '/') {
                if (result_pos < MAX_PATH_LEN - 1) {
                    result[result_pos++] = '/';
                }
                ++path_pos;
            }
        }
    }
    
    result[result_pos] = '\0';
    
    // 결과 복사
    int i = 0;
    while (result[i] && i < MAX_PATH_LEN - 1) {
        path[i] = result[i];
        ++i;
    }
    path[i] = '\0';
}

// 경로 결합
void path_join(const char* base, const char* rel, char* result, int max_len) {
    if (!base || !rel || !result) return;
    
    int i = 0;
    
    // base 복사
    if (base[0] == '/') {
        while (base[i] && i < max_len - 1) {
            result[i] = base[i];
            ++i;
        }
    } else {
        result[0] = '/';
        i = 1;
        while (base[i - 1] && i < max_len - 1) {
            result[i] = base[i - 1];
            ++i;
        }
    }
    
    // / 추가 (없으면)
    if (i > 0 && result[i - 1] != '/') {
        if (i < max_len - 1) {
            result[i++] = '/';
        }
    }
    
    // rel 복사
    int j = 0;
    if (rel[0] == '/') ++j;  // 절대 경로면 / 제거
    while (rel[j] && i < max_len - 1) {
        result[i++] = rel[j++];
    }
    
    result[i] = '\0';
    path_normalize(result);
}

