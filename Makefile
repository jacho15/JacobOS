ASM      = nasm
CC       = i686-linux-gnu-gcc
LD       = i686-linux-gnu-ld
OBJCOPY  = i686-linux-gnu-objcopy

#running the code on the qemu emulator (so i dont brick my laptop)
QEMU     = qemu-system-i386
QEMU_MEM = 32M

CFLAGS = -ffreestanding -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
         -nostartfiles -nodefaultlibs -fno-pic -Wall -Wextra -I.
LDFLAGS  = -T linker.ld

# ---- sources --------------------------------------------------------------
BOOT_SRC   = boot/boot.asm
BOOT_BIN   = build/boot.bin

# kernel_entry must be linked FIRST so its stub sits at 0x1000
KENTRY_SRC = kernel/kernel_entry.asm
KENTRY_OBJ = build/kernel/kernel_entry.o

C_SOURCES  = $(wildcard kernel/*.c drivers/*.c cpu/*.c mem/*.c fs/*.c lib/*.c)
C_OBJS     = $(patsubst %.c,build/%.o,$(C_SOURCES))

ASM_RAW    = $(wildcard cpu/*.asm kernel/*.asm)
ASM_SRCS   = $(filter-out $(KENTRY_SRC),$(ASM_RAW))
ASM_OBJS   = $(patsubst %.asm,build/%.o,$(ASM_SRCS))

KERNEL_BIN = build/kernel.bin
OS_IMAGE   = build/os-image.bin
DISK_IMG   = build/disk.img

# ---- user program (ring-3 demo) -------------------------------------------
# built freestanding, linked at the user window (8MB), then embedded into the
# kernel as a binary blob so the kernel can write it to the filesystem at boot
USER_CFLAGS = -ffreestanding -m32 -nostdlib -nostdinc -fno-builtin -fno-pic \
              -fno-asynchronous-unwind-tables -fno-stack-protector -Os -Wall -I.
USER_ELF    = build/user/hello.elf
USER_BLOB   = build/user/hello_blob.o

# ---- build rules ----------------------------------------------------------
all: $(OS_IMAGE)

build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: %.asm
	@mkdir -p $(dir $@)
	$(ASM) -f elf32 $< -o $@

$(BOOT_BIN): $(BOOT_SRC) boot/gdt.asm
	@mkdir -p $(dir $@)
	$(ASM) -f bin $< -o $@

build/user/hello.o: user/hello.c
	@mkdir -p build/user
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(USER_ELF): build/user/hello.o user/user.ld
	$(LD) -s -T user/user.ld -o $@ build/user/hello.o

$(USER_BLOB): $(USER_ELF)
	$(OBJCOPY) -I binary -O elf32-i386 -B i386 $< $@

$(KERNEL_BIN): $(KENTRY_OBJ) $(C_OBJS) $(ASM_OBJS) $(USER_BLOB)
	$(LD) -o $@ $(LDFLAGS) --oformat binary $^

$(OS_IMAGE): $(BOOT_BIN) $(KERNEL_BIN)
	cat $^ > $@
	# pad to a standard 1.44MB floppy so the BIOS sector reads never run past
	# end-of-image and the geometry is well-defined
	truncate -s 1474560 $@

# a 16MB IDE disk that persists across reboots (created once, kept around)
$(DISK_IMG):
	@mkdir -p build
	dd if=/dev/zero of=$@ bs=1M count=16 2>/dev/null

# ---- run / debug ----------------------------------------------------------
# boot disk (kernel) is the primary IDE master; the persistent FS disk is the
# primary IDE slave (hdb), which the ATA driver targets
DRIVES = -boot c \
         -drive format=raw,file=$(OS_IMAGE),if=ide,index=0,media=disk \
         -drive format=raw,file=$(DISK_IMG),if=ide,index=1,media=disk

run: all $(DISK_IMG)
	$(QEMU) -m $(QEMU_MEM) $(DRIVES)

# headless boot: mirrors the screen to stdout via COM1 (pair with `timeout`)
run-headless: all $(DISK_IMG)
	$(QEMU) -m $(QEMU_MEM) $(DRIVES) -display none -serial stdio

debug: all $(DISK_IMG)
	$(QEMU) -m $(QEMU_MEM) $(DRIVES) -serial stdio -d int,cpu_reset -no-reboot

# refresh the GitHub Pages demo: rebuild the bootable image into docs/.
# (docs/jacobos-disk.img is the committed, pre-seeded filesystem disk and is
# left untouched so the live demo keeps its starter content.)
web: all
	cp $(OS_IMAGE) docs/jacobos.img
	@echo "docs/jacobos.img updated. commit docs/ and enable Pages on /docs."

clean:
	rm -rf build

# wipe the persistent disk only when explicitly asked
clean-disk:
	rm -f $(DISK_IMG)

.PHONY: all run run-headless debug web clean clean-disk
