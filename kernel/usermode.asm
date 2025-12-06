; kernel/usermode.asm - 유저 모드 진입 함수

BITS 32

; 유저 모드로 전환하는 함수
; void enter_usermode(uint32_t entry_point, uint32_t stack_pointer)
global enter_usermode
enter_usermode:
    ; 스택에서 인자 읽기
    mov eax, [esp + 4]  ; entry_point
    mov ebx, [esp + 8]  ; stack_pointer
    
    ; 유저 모드 세그먼트 선택자
    ; 0x18 = 유저 코드 세그먼트 (Ring 3)
    ; 0x20 = 유저 데이터 세그먼트 (Ring 3)
    mov ecx, 0x20  ; 유저 데이터 세그먼트
    
    ; 인터럽트 플래그 설정 (Ring 3에서 인터럽트 허용)
    push 0x200     ; EFLAGS (IF=1)
    push 0x18      ; 유저 코드 세그먼트
    push eax       ; EIP (진입점)
    
    ; 유저 모드 스택 설정
    push ecx       ; SS (유저 데이터 세그먼트)
    push ebx       ; ESP (유저 스택 포인터)
    
    ; 세그먼트 레지스터 설정
    mov ds, ecx
    mov es, ecx
    mov fs, ecx
    mov gs, ecx
    
    ; IRET으로 유저 모드로 전환
    iretd

