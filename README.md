# JacobOS

A 32-bit x86 operating system written from scratch in C and assembly — custom
bootloader, protected-mode kernel, preemptive multitasking, paging, and a
persistent on-disk filesystem with a small shell.

**▶ Live demo (runs in your browser): https://jacho15.github.io/JacobOS/**

The demo boots the real kernel image inside [v86](https://github.com/copy/v86),
an x86 emulator compiled to WebAssembly — no install, just type `help`.

## What it does

| Area         | Implementation |
|--------------|----------------|
| **Boot**     | Custom MBR bootloader; enables A20-free protected mode via a GDT, loads the kernel over INT 13h LBA reads, jumps to C. |
| **Display**  | VGA text-mode driver with scrolling + hardware cursor; mirrored to COM1 serial for headless debugging. |
| **Interrupts** | 256-entry IDT, ISR stubs for all CPU exceptions, 8259 PIC remap, IRQ dispatch. |
| **Input**    | PS/2 keyboard driver (IRQ1) with shift/caps and a line buffer. |
| **Memory**   | Bitmap physical frame allocator, 4 MB-page paging, first-fit kernel heap (`kmalloc`/`kfree`), page-fault reporting. |
| **Tasking**  | 1:1 preemptive kernel threads, round-robin scheduler driven by the PIT timer (IRQ0), plus cooperative M:1 green threads (fibers) layered on top. |
| **User mode** | ELF loader runs static programs in **ring 3** (user GDT segments + a TSS for the ring transition); they call the kernel via `int 0x80` syscalls (`write`, `exit`, `yield`). |
| **Storage**  | Synchronous ATA PIO driver → hierarchical filesystem (inodes, directories, `.`/`..`) → write-back RAM block cache. Persists across reboots. |
| **Shell**    | `help`, `ls`, `cd`, `pwd`, `mkdir`, `touch`, `cat`, `cp`, `mv`, `echo > file` (and `>>` to append), `rm` (and `rm -r`), `exec`, `gt`, `ps`, `spawn`, `stress`, `meminfo`, `uptime`. |

## Layout

```
boot/     bootloader + GDT (NASM)
cpu/      IDT, ISRs, PIC, runtime GDT + TSS, int 0x80 syscalls, port I/O, types
drivers/  screen, serial, keyboard, timer, ATA
mem/      physical frame allocator, paging, kernel heap
fs/       filesystem + block cache
kernel/   entry, main, tasks/scheduler, context switch, green threads,
          ELF loader + ring-3 exec, shell
lib/      freestanding string/mem/itoa helpers
user/     a tiny freestanding ring-3 demo program (embedded as /hello)
docs/     the browser demo (GitHub Pages)
```

## Build & run locally

Requires a 32-bit cross toolchain and QEMU:

```sh
# Debian/Ubuntu (or WSL)
sudo apt-get install nasm gcc-i686-linux-gnu binutils-i686-linux-gnu qemu-system-x86

make run          # build and boot in QEMU
make run-headless # boot with serial mirrored to the terminal (no window)
make debug        # boot with interrupt/reset logging
```

The kernel boots from `build/os-image.bin` (IDE master) and stores its
filesystem on `build/disk.img` (IDE slave), which is created automatically and
persists between runs. `make clean-disk` wipes it.

## The browser demo

`docs/` is a static GitHub Pages site. `index.html` loads v86 from a pinned CDN
and boots two images shipped alongside it:

- `jacobos.img` — the bootable kernel image (`hda`)
- `jacobos-disk.img` — a pre-seeded filesystem disk (`hdb`)

To refresh the demo after kernel changes:

```sh
make web   # rebuilds and copies the image into docs/jacobos.img
```

Enable Pages under **Settings → Pages → Deploy from branch → `/docs`**, or let
the included `.github/workflows/pages.yml` build and deploy on every push.

> Note: the demo persists across reloads by saving the full machine state
> (including the filesystem disk) to the browser's **IndexedDB** — automatically
> when you leave the tab, or via the **Save session** button — and restoring it on
> your next visit. **Reset disk** clears it for a clean boot. Local QEMU runs
> persist fully via `build/disk.img`.
