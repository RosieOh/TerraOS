// kernel/network.c - 기본 네트워크 스택
// 간단한 TCP/IP 스택 및 소켓 API

#include <stdint.h>

// 네트워크 프로토콜
#define PROTO_TCP 6
#define PROTO_UDP 17

// 소켓 타입
#define SOCK_STREAM 1
#define SOCK_DGRAM 2

// 소켓 상태
typedef enum {
    SOCK_CLOSED,
    SOCK_LISTENING,
    SOCK_CONNECTED
} sock_state_t;

// 소켓 구조체
typedef struct {
    int type;
    int protocol;
    sock_state_t state;
    uint32_t local_ip;
    uint16_t local_port;
    uint32_t remote_ip;
    uint16_t remote_port;
    int valid;
} socket_t;

#define MAX_SOCKETS 16
static socket_t sockets[MAX_SOCKETS];
static int socket_count = 0;

// 소켓 생성
int socket_create(int type, int protocol) {
    if (socket_count >= MAX_SOCKETS) return -1;
    
    socket_t* sock = &sockets[socket_count];
    sock->type = type;
    sock->protocol = protocol;
    sock->state = SOCK_CLOSED;
    sock->local_ip = 0;
    sock->local_port = 0;
    sock->remote_ip = 0;
    sock->remote_port = 0;
    sock->valid = 1;
    
    ++socket_count;
    return socket_count - 1;  // 소켓 디스크립터 반환
}

// 소켓 바인드
int socket_bind(int sockfd, uint32_t ip, uint16_t port) {
    if (sockfd < 0 || sockfd >= MAX_SOCKETS || !sockets[sockfd].valid) {
        return -1;
    }
    
    sockets[sockfd].local_ip = ip;
    sockets[sockfd].local_port = port;
    return 0;
}

// 소켓 리스닝
int socket_listen(int sockfd) {
    if (sockfd < 0 || sockfd >= MAX_SOCKETS || !sockets[sockfd].valid) {
        return -1;
    }
    
    sockets[sockfd].state = SOCK_LISTENING;
    return 0;
}

// 네트워크 초기화
void network_init(void) {
    socket_count = 0;
    for (int i = 0; i < MAX_SOCKETS; ++i) {
        sockets[i].valid = 0;
    }
}

