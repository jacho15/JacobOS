ASM      = nasm
CC       = i686-linux-gnu-gcc
LD       = i686-linux-gnu-ld

#running the code on the qemu emulator (so i dont brick my laptop)
QEMU     = qemu-system-i386

CFLAGS = -ffreestanding -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -fno-pic -Wall -Wextra
LDFLAGS  = -T linker.ld  

BOOT_SRC     = boot/boot.asm
BOOT_BIN     = build/boot.bin

KENTRY_SRC = kernel/kernel_entry.asm
KENTRY_OBJ = build/kernel_entry.o

KERNEL_SRC = kernel/kernel.c
KERNEL_OBJ = build/kernel.o

KERNEL_BIN   = build/kernel.bin
OS_IMAGE     = build/os-image.bin

all: dirs $(OS_IMAGE)
dirs:
	@mkdir -p build

$(BOOT_BIN): $(BOOT_SRC)
	$(ASM) -f bin $< -o $@ 

$(KENTRY_OBJ): $(KENTRY_SRC)
	$(ASM) -f elf32 $< -o $@
 
$(KERNEL_OBJ): $(KERNEL_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_BIN): $(KENTRY_OBJ) $(KERNEL_OBJ)
	$(LD) -o $@ -T linker.ld --oformat binary $^
 
$(OS_IMAGE): $(BOOT_BIN) $(KERNEL_BIN)
	cat $^ > $@

run: all
	$(QEMU) -drive format=raw,file=$(OS_IMAGE),index=0,if=floppy

debug: all
	$(QEMU) -drive format=raw,file=$(OS_IMAGE),index=0,if=floppy -d int,cpu_reset -no-reboot

clean:
	rm -rf build

.PHONY: all dirs run debug clean