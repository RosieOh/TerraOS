/* kernel/linker.ld - 32비트 커널 링커 스크립트 */

OUTPUT_FORMAT(elf32-i386)
ENTRY(_start)

SECTIONS
{
    /* GRUB이 이 주소 근처에 커널을 로드한다는 가정 (일반적인 예제 값) */
    . = 1M;

    .text ALIGN(4K) : {
        *(.multiboot)
        *(.text*)
    }

    .rodata ALIGN(4K) : {
        *(.rodata*)
    }

    .data ALIGN(4K) : {
        *(.data*)
    }

    .bss ALIGN(4K) : {
        *(COMMON)
        *(.bss*)
    }

    /* 커널 끝 위치를 외부에서 참조할 수 있도록 심볼 제공 */
    _kernel_end = .;
}


