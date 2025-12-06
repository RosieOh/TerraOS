// user/hello.c - 간단한 유저 모드 프로그램 예제
// terraOS에서 실행되는 첫 번째 프로그램

// 시스템 콜 번호 정의 (kernel/syscall.c와 동일)
#define SYS_WRITE   1
#define SYS_READ    2
#define SYS_EXIT    3
#define SYS_GETPID  5

// 시스템 콜 래퍼 함수
static inline int syscall(int num, int arg1, int arg2, int arg3) {
    int result;
    __asm__ volatile(
        "int $0x80"
        : "=a"(result)
        : "a"(num), "b"(arg1), "c"(arg2), "d"(arg3)
    );
    return result;
}

// 간단한 문자열 길이 계산
static int strlen(const char* s) {
    int len = 0;
    while (s[len]) ++len;
    return len;
}

// 메인 함수
void _start(void) {
    const char* msg = "Hello from user mode program!\n";
    const char* msg2 = "This program is running in Ring 3.\n";
    const char* msg3 = "System call test: PID = ";
    
    // SYS_WRITE로 메시지 출력
    syscall(SYS_WRITE, 1, (int)msg, strlen(msg));
    syscall(SYS_WRITE, 1, (int)msg2, strlen(msg2));
    syscall(SYS_WRITE, 1, (int)msg3, strlen(msg3));
    
    // PID 출력 (간단히 숫자로)
    int pid = syscall(SYS_GETPID, 0, 0, 0);
    char pid_str[16];
    int i = 0;
    if (pid == 0) {
        pid_str[i++] = '0';
    } else {
        char temp[16];
        int j = 0;
        while (pid > 0) {
            temp[j++] = '0' + (pid % 10);
            pid /= 10;
        }
        while (j-- > 0) {
            pid_str[i++] = temp[j];
        }
    }
    pid_str[i++] = '\n';
    pid_str[i] = '\0';
    
    syscall(SYS_WRITE, 1, (int)pid_str, i);
    
    // 프로그램 종료
    syscall(SYS_EXIT, 0, 0, 0);
}

