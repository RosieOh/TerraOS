; kernel/interrupts.asm - ISR/IRQ 어셈블리 스텁

BITS 32

extern isr_handler
extern irq_handler
extern syscall_handler_c

%macro ISR_NOERR 1
global isr%1
isr%1:
    cli
    push dword 0        ; 가짜 에러 코드
    push dword %1       ; 인터럽트 번호
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10        ; 커널 데이터 세그먼트 (GRUB 기본 GDT 가정)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call isr_handler

    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8          ; 에러 코드 + 인터럽트 번호
    sti
    iretd
%endmacro

%macro ISR_ERR 1
global isr%1
isr%1:
    cli
    push dword %1       ; 인터럽트 번호
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call isr_handler

    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 4          ; 인터럽트 번호
    sti
    iretd
%endmacro

%macro IRQ 2
global irq%1
irq%1:
    cli
    push dword 0        ; 가짜 에러 코드
    push dword %2       ; 인터럽트 번호 (PIC 리맵 이후 번호)
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call irq_handler

    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8
    sti
    iretd
%endmacro

; 예시로 0(division by zero), 13(GPF), 14(page fault)만 정의
ISR_NOERR 0
ISR_ERR   13
ISR_ERR   14

; 시스템 콜 인터럽트 (0x80) - 특별 처리 필요
global isr128
isr128:
    cli
    push dword 0        ; 가짜 에러 코드
    push dword 128      ; 인터럽트 번호
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 시스템 콜 핸들러 호출 (레지스터 값 전달)
    ; EAX = syscall 번호, EBX = arg1, ECX = arg2, EDX = arg3
    push edx            ; arg3
    push ecx            ; arg2
    push ebx            ; arg1
    push eax            ; syscall 번호
    call syscall_handler_c
    add esp, 16         ; 스택 정리
    
    ; 결과를 EAX에 저장 (스택의 EAX 위치에)
    mov [esp + 44], eax  ; pusha로 저장된 EAX 위치에 결과 저장

    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8          ; 에러 코드 + 인터럽트 번호
    sti
    iretd

; PIC를 0x20~ 범위로 리맵한다고 가정했을 때
; IRQ0 = 32 (타이머), IRQ1 = 33 (키보드)
IRQ 0, 32
IRQ 1, 33

; 컨텍스트 스위칭 함수
; void context_switch(task_context_t* from, task_context_t* to)
; 주의: 이 함수는 인터럽트 핸들러 내부에서 호출되므로,
; 스택에는 이미 모든 레지스터가 push되어 있음
global context_switch
context_switch:
    ; 현재 스택 구조:
    ; [esp+0] = 리턴 주소
    ; [esp+4] = from 포인터
    ; [esp+8] = to 포인터
    ; 그 아래는 인터럽트 핸들러가 push한 레지스터들
    
    ; from 컨텍스트에 현재 스택 포인터 저장
    mov eax, [esp + 4]  ; from 포인터
    mov [eax + 12], esp  ; ESP 저장 (현재 스택 포인터)
    
    ; to 컨텍스트의 스택 포인터로 전환
    mov eax, [esp + 8]  ; to 포인터
    mov esp, [eax + 12]  ; 새 스택 포인터
    
    ; 새 태스크의 스택에서 레지스터 복원은 인터럽트 핸들러가 처리
    ret

; 태스크 진입점 래퍼는 tasks.c에서 C 함수로 정의됨
; 여기서는 정의하지 않음


