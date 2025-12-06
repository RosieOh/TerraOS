# user/Makefile - 유저 모드 프로그램 빌드

CC := i686-elf-gcc
AS := nasm
LD := i686-elf-gcc

CFLAGS := -std=gnu99 -ffreestanding -O2 -Wall -Wextra -m32 -nostdlib -fno-builtin
LDFLAGS := -ffreestanding -nostdlib -m32 -Wl,--build-id=none

# 간단한 링커 스크립트 (인라인)
LINKER_SCRIPT := -Wl,-Ttext=0x1000000

HELLO_OBJ := hello.o
HELLO_BIN := hello.elf

.PHONY: all clean

all: $(HELLO_BIN)

$(HELLO_OBJ): hello.c
	$(CC) $(CFLAGS) -c $< -o $@

$(HELLO_BIN): $(HELLO_OBJ)
	$(LD) $(LDFLAGS) $(LINKER_SCRIPT) $< -o $@

clean:
	rm -f $(HELLO_OBJ) $(HELLO_BIN)

