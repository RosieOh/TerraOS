# Makefile - terraOS 32비트 hobby OS 최소 빌드/실행

TARGET       := terraOS
BUILD_DIR    := build
ISO_DIR      := $(BUILD_DIR)/iso
BIN_DIR      := $(BUILD_DIR)/bin
USER_DIR     := user
TOOLS_DIR    := tools

CC           := i686-elf-gcc
AS           := nasm
LD           := i686-elf-gcc

CFLAGS       := -std=gnu99 -ffreestanding -O2 -Wall -Wextra -m32
LDFLAGS      := -ffreestanding -O2 -nostdlib -m32 -Wl,-T,kernel/linker.ld

QEMU         := qemu-system-i386

BOOT_SRCS    := boot/boot.asm
KERNEL_SRCS  := kernel/kernel.c kernel/interrupts.asm kernel/paging.c kernel/ramfs.c kernel/tasks.c kernel/syscall.c kernel/gdt.c kernel/usermode.asm kernel/elf.c kernel/ramfs_hello.c kernel/memory.c kernel/keyboard.c kernel/log.c kernel/time.c kernel/env.c kernel/vfs.c kernel/signal.c kernel/pipe.c kernel/network.c kernel/graphics.c kernel/smp.c kernel/shell.c kernel/path.c kernel/devfs.c kernel/heap.c kernel/exec.c kernel/debug.c kernel/monitor.c kernel/design.c

BOOT_OBJ     := $(BIN_DIR)/boot.o
KERNEL_OBJ   := $(BIN_DIR)/kernel.o $(BIN_DIR)/interrupts.o $(BIN_DIR)/paging.o $(BIN_DIR)/ramfs.o $(BIN_DIR)/tasks.o $(BIN_DIR)/syscall.o $(BIN_DIR)/gdt.o $(BIN_DIR)/usermode.o $(BIN_DIR)/elf.o $(BIN_DIR)/ramfs_hello.o $(BIN_DIR)/memory.o $(BIN_DIR)/keyboard.o $(BIN_DIR)/log.o $(BIN_DIR)/time.o $(BIN_DIR)/env.o $(BIN_DIR)/vfs.o $(BIN_DIR)/signal.o $(BIN_DIR)/pipe.o $(BIN_DIR)/network.o $(BIN_DIR)/graphics.o $(BIN_DIR)/smp.o $(BIN_DIR)/shell.o $(BIN_DIR)/path.o $(BIN_DIR)/devfs.o $(BIN_DIR)/heap.o $(BIN_DIR)/exec.o $(BIN_DIR)/debug.o $(BIN_DIR)/monitor.o $(BIN_DIR)/design.o
KERNEL_ELF   := $(BIN_DIR)/kernel.elf

GRUB_CFG_DIR := $(ISO_DIR)/boot/grub
GRUB_CFG     := $(GRUB_CFG_DIR)/grub.cfg
ISO_IMAGE    := $(BUILD_DIR)/$(TARGET).iso

USER_HELLO_ELF := $(USER_DIR)/hello.elf
USER_HELLO_C := kernel/ramfs_hello.c

.PHONY: all run run-iso iso clean distclean toolchain-info user-programs

# 기본은 GRUB 없이도 바로 QEMU에 커널을 올려서 테스트할 수 있도록
all: user-programs $(KERNEL_ELF)

# 유저 프로그램 빌드
user-programs:
	$(MAKE) -C $(USER_DIR)

# ELF 파일을 C 배열로 변환
$(USER_HELLO_C): $(USER_HELLO_ELF)
	python3 $(TOOLS_DIR)/elf_to_c.py $(USER_HELLO_ELF) hello_elf > $@

toolchain-info:
	@echo "Using compiler: $(CC)"
	@$(CC) --version || echo "WARNING: cross-compiler $(CC) 가 설치되지 않았을 수 있습니다."

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(ISO_DIR):
	mkdir -p $(ISO_DIR)/boot/grub

$(BOOT_OBJ): $(BOOT_SRCS) | $(BIN_DIR)
	$(AS) -f elf32 $< -o $@

$(BIN_DIR)/kernel.o: kernel/kernel.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/interrupts.o: kernel/interrupts.asm | $(BIN_DIR)
	$(AS) -f elf32 $< -o $@

$(BIN_DIR)/paging.o: kernel/paging.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/ramfs.o: kernel/ramfs.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/tasks.o: kernel/tasks.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/syscall.o: kernel/syscall.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/gdt.o: kernel/gdt.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/usermode.o: kernel/usermode.asm | $(BIN_DIR)
	$(AS) -f elf32 $< -o $@

$(BIN_DIR)/elf.o: kernel/elf.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/ramfs_hello.o: kernel/ramfs_hello.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/memory.o: kernel/memory.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/keyboard.o: kernel/keyboard.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/log.o: kernel/log.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/time.o: kernel/time.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/env.o: kernel/env.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/vfs.o: kernel/vfs.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/signal.o: kernel/signal.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/pipe.o: kernel/pipe.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/network.o: kernel/network.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/graphics.o: kernel/graphics.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/smp.o: kernel/smp.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/shell.o: kernel/shell.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/path.o: kernel/path.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/devfs.o: kernel/devfs.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/heap.o: kernel/heap.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/exec.o: kernel/exec.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/debug.o: kernel/debug.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/monitor.o: kernel/monitor.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/design.o: kernel/design.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_ELF): $(BOOT_OBJ) $(KERNEL_OBJ) | user-programs
	$(LD) $(LDFLAGS) $(BOOT_OBJ) $(KERNEL_OBJ) -o $@

$(GRUB_CFG): | $(ISO_DIR)
	@echo "set timeout=0"                       >  $(GRUB_CFG)
	@echo "set default=0"                      >> $(GRUB_CFG)
	@echo ""                                   >> $(GRUB_CFG)
	@echo "menuentry \"terraOS\" {"           >> $(GRUB_CFG)
	@echo "    multiboot /boot/kernel.elf"    >> $(GRUB_CFG)
	@echo "    boot"                          >> $(GRUB_CFG)
	@echo "}"                                 >> $(GRUB_CFG)

$(ISO_IMAGE): $(KERNEL_ELF) $(GRUB_CFG)
	cp $(KERNEL_ELF) $(ISO_DIR)/boot/kernel.elf
	grub-mkrescue -o $(ISO_IMAGE) $(ISO_DIR)

iso: $(ISO_IMAGE)

# GRUB/ISO 없이, QEMU가 multiboot 커널을 직접 로드하게 하는 실행 방법
# macOS Homebrew QEMU는 보통 cocoa 디스플레이를 사용하므로, -display cocoa 로 명시
# 창 크기는 QEMU 창을 직접 리사이즈하거나, 필요하면 -full-screen 옵션을 추가해서 전체 화면 사용 가능
run: $(KERNEL_ELF)
	$(QEMU) -display cocoa -kernel $(KERNEL_ELF)

# GRUB ISO를 이용해 부팅하고 싶을 때 사용할 별도 실행 타겟
run-iso: $(ISO_IMAGE)
	$(QEMU) -display cocoa -cdrom $(ISO_IMAGE)

clean:
	rm -rf $(BIN_DIR)

distclean: clean
	rm -rf $(BUILD_DIR)


