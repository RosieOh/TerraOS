// kernel/elf.c - ELF 실행 파일 로더
// RAM 디스크에서 ELF 파일을 읽어 프로세스로 실행

#include <stdint.h>

// ELF 헤더 구조체
#define EI_NIDENT 16

typedef struct {
    unsigned char e_ident[EI_NIDENT];
    uint16_t      e_type;
    uint16_t      e_machine;
    uint32_t      e_version;
    uint32_t      e_entry;
    uint32_t      e_phoff;
    uint32_t      e_shoff;
    uint32_t      e_flags;
    uint16_t      e_ehsize;
    uint16_t      e_phentsize;
    uint16_t      e_phnum;
    uint16_t      e_shentsize;
    uint16_t      e_shnum;
    uint16_t      e_shstrndx;
} elf_header_t;

// 프로그램 헤더 구조체
typedef struct {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} elf_program_header_t;

#define PT_LOAD 1

// ELF 매직 넘버 확인
static int elf_check_magic(elf_header_t* header) {
    return (header->e_ident[0] == 0x7F &&
            header->e_ident[1] == 'E' &&
            header->e_ident[2] == 'L' &&
            header->e_ident[3] == 'F');
}

// ELF 파일 로드 및 실행
int elf_load_and_execute(const char* filename) {
    // RAM 디스크에서 파일 읽기
    extern int ramfs_read(const char* name, const char** out_data, uint32_t* out_size);
    const char* file_data = 0;
    uint32_t file_size = 0;
    
    if (!ramfs_read(filename, &file_data, &file_size)) {
        return -1;  // 파일을 찾을 수 없음
    }
    
    if (file_size < sizeof(elf_header_t)) {
        return -2;  // 파일이 너무 작음
    }
    
    elf_header_t* header = (elf_header_t*)file_data;
    
    // ELF 매직 넘버 확인
    if (!elf_check_magic(header)) {
        return -3;  // 유효하지 않은 ELF 파일
    }
    
    // 32비트 ELF 확인
    if (header->e_ident[4] != 1) {  // ELFCLASS32
        return -4;  // 64비트 ELF는 지원하지 않음
    }
    
    // 리틀 엔디안 확인
    if (header->e_ident[5] != 1) {  // ELFDATA2LSB
        return -5;  // 빅 엔디안은 지원하지 않음
    }
    
    // i386 머신 확인
    if (header->e_machine != 3) {  // EM_386
        return -6;  // i386이 아님
    }
    
    // 프로그램 헤더 읽기
    if (header->e_phoff == 0 || header->e_phnum == 0) {
        return -7;  // 프로그램 헤더가 없음
    }
    
    // 메모리에 세그먼트 로드
    elf_program_header_t* ph = (elf_program_header_t*)(file_data + header->e_phoff);
    
    for (uint16_t i = 0; i < header->e_phnum; ++i) {
        if (ph[i].p_type == PT_LOAD) {
            // 로드 가능한 세그먼트
            uint32_t vaddr = ph[i].p_vaddr;
            uint32_t memsz = ph[i].p_memsz;
            uint32_t filesz = ph[i].p_filesz;
            const char* src = file_data + ph[i].p_offset;
            
            // 메모리 할당 (간단히 vaddr에 직접 복사 - 실제로는 페이지 매핑 필요)
            // 지금은 간단히 vaddr이 유효한 주소라고 가정
            char* dest = (char*)vaddr;
            
            // 데이터 복사
            for (uint32_t j = 0; j < filesz; ++j) {
                dest[j] = src[j];
            }
            
            // BSS 영역 초기화 (memsz > filesz인 경우)
            for (uint32_t j = filesz; j < memsz; ++j) {
                dest[j] = 0;
            }
        }
    }
    
    // 유저 모드 태스크 생성 및 실행
    // tasks.c의 타입 정의 사용
    typedef struct task task_t;
    extern task_t* task_create_usermode(const char* name, void (*entry)(void));
    extern void preemptive_enable(void);
    
    // 엔트리 포인트 저장 (전역 변수로)
    extern uint32_t elf_entry_point;
    elf_entry_point = header->e_entry;
    
    // 래퍼 함수는 별도로 정의
    extern void elf_entry_wrapper(void);
    
    task_t* task = task_create_usermode(filename, elf_entry_wrapper);
    if (!task) {
        return -8;  // 태스크 생성 실패
    }
    
    // 태스크를 실행 큐에 추가 (스케줄러가 처리)
    preemptive_enable();
    
    return 0;  // 성공
}

// ELF 엔트리 포인트 저장용 전역 변수
uint32_t elf_entry_point = 0;

// ELF 진입점 래퍼
void elf_entry_wrapper(void) {
    extern uint32_t elf_entry_point;
    void (*entry)(void) = (void (*)(void))elf_entry_point;
    
    if (entry) {
        entry();
    }
    
    // 프로그램 종료
    extern void task_exit_current(void);
    task_exit_current();
}

