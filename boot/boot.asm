; boot/boot.asm - Multiboot 호환 32비트 커널 진입용 어셈블리
; GRUB이 이 커널을 로드하고 여기로 점프한다고 가정한다.

BITS 32

section .multiboot
align 4
    ; Multiboot 헤더 (버전 1)
    MULTIBOOT_MAGIC      equ 0x1BADB002
    MULTIBOOT_FLAGS      equ 0x00000003        ; 메모리 정보 + 페이지 정렬
    MULTIBOOT_CHECKSUM   equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

    dd MULTIBOOT_MAGIC
    dd MULTIBOOT_FLAGS
    dd MULTIBOOT_CHECKSUM

section .text
global _start
extern kmain

_start:
    ; GRUB이 이미 보호 모드(32비트)로 넘어온 상태라고 가정
    ; 스택 초기화
    mov esp, stack_top

    ; Multiboot 정보 구조체 포인터 전달 (GRUB이 EAX, EBX에 넣어줌)
    ; 여기서는 단순히 kmain으로 점프만 하고, 인자는 나중에 활용 가능
    push ebx         ; multiboot_info*
    push eax         ; multiboot_magic
    call kmain

    ; kmain이 리턴하지 않는 것이 일반적이지만,
    ; 혹시 리턴하면 그냥 무한 루프로 빠진다.
.hang:
    cli
    hlt
    jmp .hang

section .bss
align 16
stack_bottom:
    resb 4096
stack_top:


