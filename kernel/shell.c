// kernel/shell.c - 고급 쉘 기능
// 파이프, 리다이렉션, 백그라운드 프로세스 파싱

#include <stdint.h>

#define MAX_TOKENS 16
#define MAX_TOKEN_LEN 64
#define MAX_JOBS 8

// 명령어 토큰
typedef struct {
    char tokens[MAX_TOKENS][MAX_TOKEN_LEN];
    int token_count;
    int has_pipe;      // 파이프 여부
    int has_redirect_in;   // < 리다이렉션
    int has_redirect_out;  // > 리다이렉션
    int background;    // 백그라운드 실행 (&)
    char redirect_in_file[MAX_TOKEN_LEN];
    char redirect_out_file[MAX_TOKEN_LEN];
} command_t;

// 작업 (백그라운드 프로세스)
typedef struct {
    uint32_t pid;
    char command[128];
    int valid;
} job_t;

static job_t jobs[MAX_JOBS];
static int job_count = 0;

// 명령어 파싱
int shell_parse_command(const char* line, command_t* cmd) {
    if (!line || !cmd) return -1;
    
    cmd->token_count = 0;
    cmd->has_pipe = 0;
    cmd->has_redirect_in = 0;
    cmd->has_redirect_out = 0;
    cmd->background = 0;
    cmd->redirect_in_file[0] = '\0';
    cmd->redirect_out_file[0] = '\0';
    
    int i = 0;
    int token_start = 0;
    int in_token = 0;
    
    while (line[i] && cmd->token_count < MAX_TOKENS) {
        char c = line[i];
        
        if (c == ' ' || c == '\t') {
            if (in_token) {
                // 토큰 종료
                int len = i - token_start;
                if (len > 0 && len < MAX_TOKEN_LEN) {
                    for (int j = 0; j < len; ++j) {
                        cmd->tokens[cmd->token_count][j] = line[token_start + j];
                    }
                    cmd->tokens[cmd->token_count][len] = '\0';
                    ++cmd->token_count;
                }
                in_token = 0;
            }
        } else if (c == '|') {
            if (in_token) {
                // 현재 토큰 종료
                int len = i - token_start;
                if (len > 0 && len < MAX_TOKEN_LEN) {
                    for (int j = 0; j < len; ++j) {
                        cmd->tokens[cmd->token_count][j] = line[token_start + j];
                    }
                    cmd->tokens[cmd->token_count][len] = '\0';
                    ++cmd->token_count;
                }
                in_token = 0;
            }
            cmd->has_pipe = 1;
        } else if (c == '<') {
            if (in_token) {
                int len = i - token_start;
                if (len > 0 && len < MAX_TOKEN_LEN) {
                    for (int j = 0; j < len; ++j) {
                        cmd->tokens[cmd->token_count][j] = line[token_start + j];
                    }
                    cmd->tokens[cmd->token_count][len] = '\0';
                    ++cmd->token_count;
                }
                in_token = 0;
            }
            cmd->has_redirect_in = 1;
            // 다음 토큰이 파일명
            ++i;
            while (line[i] == ' ' || line[i] == '\t') ++i;
            token_start = i;
            in_token = 1;
        } else if (c == '>') {
            if (in_token) {
                int len = i - token_start;
                if (len > 0 && len < MAX_TOKEN_LEN) {
                    for (int j = 0; j < len; ++j) {
                        cmd->tokens[cmd->token_count][j] = line[token_start + j];
                    }
                    cmd->tokens[cmd->token_count][len] = '\0';
                    ++cmd->token_count;
                }
                in_token = 0;
            }
            cmd->has_redirect_out = 1;
            // 다음 토큰이 파일명
            ++i;
            while (line[i] == ' ' || line[i] == '\t') ++i;
            token_start = i;
            in_token = 1;
        } else if (c == '&') {
            if (in_token) {
                int len = i - token_start;
                if (len > 0 && len < MAX_TOKEN_LEN) {
                    for (int j = 0; j < len; ++j) {
                        cmd->tokens[cmd->token_count][j] = line[token_start + j];
                    }
                    cmd->tokens[cmd->token_count][len] = '\0';
                    ++cmd->token_count;
                }
                in_token = 0;
            }
            cmd->background = 1;
        } else {
            if (!in_token) {
                token_start = i;
                in_token = 1;
            }
        }
        ++i;
    }
    
    // 마지막 토큰 처리
    if (in_token && cmd->token_count < MAX_TOKENS) {
        int len = i - token_start;
        if (len > 0 && len < MAX_TOKEN_LEN) {
            for (int j = 0; j < len; ++j) {
                cmd->tokens[cmd->token_count][j] = line[token_start + j];
            }
            cmd->tokens[cmd->token_count][len] = '\0';
            
            // 리다이렉션 파일명인지 확인
            if (cmd->has_redirect_in && cmd->redirect_in_file[0] == '\0') {
                for (int j = 0; j < len && j < MAX_TOKEN_LEN - 1; ++j) {
                    cmd->redirect_in_file[j] = cmd->tokens[cmd->token_count][j];
                }
                cmd->redirect_in_file[len] = '\0';
            } else if (cmd->has_redirect_out && cmd->redirect_out_file[0] == '\0') {
                for (int j = 0; j < len && j < MAX_TOKEN_LEN - 1; ++j) {
                    cmd->redirect_out_file[j] = cmd->tokens[cmd->token_count][j];
                }
                cmd->redirect_out_file[len] = '\0';
            } else {
                ++cmd->token_count;
            }
        }
    }
    
    return 0;
}

// 작업 추가
int shell_add_job(uint32_t pid, const char* command) {
    if (job_count >= MAX_JOBS) return -1;
    
    jobs[job_count].pid = pid;
    int i = 0;
    while (command[i] && i < 127) {
        jobs[job_count].command[i] = command[i];
        ++i;
    }
    jobs[job_count].command[i] = '\0';
    jobs[job_count].valid = 1;
    ++job_count;
    
    return job_count - 1;
}

// 작업 목록
int shell_list_jobs(job_t** out_jobs) {
    if (out_jobs) {
        *out_jobs = jobs;
    }
    return job_count;
}

// 작업 제거
int shell_remove_job(uint32_t pid) {
    for (int i = 0; i < job_count; ++i) {
        if (jobs[i].valid && jobs[i].pid == pid) {
            jobs[i].valid = 0;
            return 0;
        }
    }
    return -1;
}

// 작업 초기화
void shell_init(void) {
    job_count = 0;
    for (int i = 0; i < MAX_JOBS; ++i) {
        jobs[i].valid = 0;
    }
}

