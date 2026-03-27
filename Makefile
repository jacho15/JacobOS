ASM      = nasm
CC       = i686-elf-gcc
LD       = i686-elf-ld

#running the code on the qemu emulator (so i dont brick my laptop)
QEMU     = qemu-system-i386

LDFLAGS  = -T linker.ld  

BOOT_SRC     = boot/boot.asm
BOOT_BIN     = build/boot.bin

KERNEL_BIN   = build/kernel.bin
OS_IMAGE     = build/os-image.bin

all: dirs boot
dirs:
	@mkdir -p build
boot: dirs $(BOOT_BIN)

$(BOOT_BIN): $(BOOT_SRC)
	$(ASM) -f bin $< -o $@ 

run: all
	$(QEMU) -fda $(BOOT_BIN)

debug: all
	$(QEMU) -fda $(BOOT_BIN) -d int,cpu_reset -no-reboot

clean:
	rm -rf build

.PHONY: all dirs boot run debug clean