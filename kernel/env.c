// kernel/env.c - 환경 변수 시스템
// 쉘 변수 저장/읽기

#include <stdint.h>

#define MAX_ENV_VARS 64
#define MAX_ENV_NAME_LEN 64
#define MAX_ENV_VALUE_LEN 256

typedef struct {
    char name[MAX_ENV_NAME_LEN];
    char value[MAX_ENV_VALUE_LEN];
    int valid;
} env_var_t;

static env_var_t env_vars[MAX_ENV_VARS];
static int env_count = 0;

// 문자열 비교
static int str_compare(const char* a, const char* b) {
    while (*a && *b && *a == *b) {
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

// 문자열 복사
static void str_copy(char* dest, const char* src, int max_len) {
    int i = 0;
    while (src[i] && i < max_len - 1) {
        dest[i] = src[i];
        ++i;
    }
    dest[i] = '\0';
}

// 환경 변수 설정
int env_set(const char* name, const char* value) {
    if (!name || !value) return -1;
    
    // 기존 변수 찾기
    for (int i = 0; i < env_count; ++i) {
        if (env_vars[i].valid && str_compare(env_vars[i].name, name)) {
            str_copy(env_vars[i].value, value, MAX_ENV_VALUE_LEN);
            return 0;
        }
    }
    
    // 새 변수 추가
    if (env_count >= MAX_ENV_VARS) return -1;
    
    str_copy(env_vars[env_count].name, name, MAX_ENV_NAME_LEN);
    str_copy(env_vars[env_count].value, value, MAX_ENV_VALUE_LEN);
    env_vars[env_count].valid = 1;
    ++env_count;
    return 0;
}

// 환경 변수 가져오기
const char* env_get(const char* name) {
    if (!name) return 0;
    
    for (int i = 0; i < env_count; ++i) {
        if (env_vars[i].valid && str_compare(env_vars[i].name, name)) {
            return env_vars[i].value;
        }
    }
    return 0;
}

// 환경 변수 삭제
int env_unset(const char* name) {
    if (!name) return -1;
    
    for (int i = 0; i < env_count; ++i) {
        if (env_vars[i].valid && str_compare(env_vars[i].name, name)) {
            env_vars[i].valid = 0;
            return 0;
        }
    }
    return -1;
}

// 모든 환경 변수 목록
int env_list(env_var_t** out_vars) {
    if (out_vars) {
        *out_vars = env_vars;
    }
    return env_count;
}

// 환경 변수 초기화
void env_init(void) {
    env_count = 0;
    for (int i = 0; i < MAX_ENV_VARS; ++i) {
        env_vars[i].valid = 0;
    }
    
    // 기본 환경 변수 설정
    env_set("PATH", "/");
    env_set("SHELL", "terraOS");
    env_set("HOME", "/");
}

