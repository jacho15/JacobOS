#include "cpu/isr.h"
#include "cpu/idt.h"
#include "cpu/ports.h"
#include "drivers/screen.h"

//assembly stubs from interrupt.asm
extern void isr0();  extern void isr1();  extern void isr2();  extern void isr3();
extern void isr4();  extern void isr5();  extern void isr6();  extern void isr7();
extern void isr8();  extern void isr9();  extern void isr10(); extern void isr11();
extern void isr12(); extern void isr13(); extern void isr14(); extern void isr15();
extern void isr16(); extern void isr17(); extern void isr18(); extern void isr19();
extern void isr20(); extern void isr21(); extern void isr22(); extern void isr23();
extern void isr24(); extern void isr25(); extern void isr26(); extern void isr27();
extern void isr28(); extern void isr29(); extern void isr30(); extern void isr31();
extern void irq0();  extern void irq1();  extern void irq2();  extern void irq3();
extern void irq4();  extern void irq5();  extern void irq6();  extern void irq7();
extern void irq8();  extern void irq9();  extern void irq10(); extern void irq11();
extern void irq12(); extern void irq13(); extern void irq14(); extern void irq15();

static isr_t interrupt_handlers[IDT_ENTRIES];

static const char *exception_messages[] = {
    "Division By Zero", "Debug", "Non Maskable Interrupt", "Breakpoint",
    "Into Detected Overflow", "Out of Bounds", "Invalid Opcode",
    "No Coprocessor", "Double Fault", "Coprocessor Segment Overrun",
    "Bad TSS", "Segment Not Present", "Stack Fault", "General Protection Fault",
    "Page Fault", "Unknown Interrupt", "Coprocessor Fault", "Alignment Check",
    "Machine Check", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved"
};

//reprogram the 8259 PICs so IRQ0..15 map to vectors 0x20..0x2F instead of
//colliding with the CPU exception vectors
static void pic_remap(void) {
    outb(0x20, 0x11); io_wait(); //start init, master
    outb(0xA0, 0x11); io_wait(); //start init, slave
    outb(0x21, 0x20); io_wait(); //master vector offset -> 0x20
    outb(0xA1, 0x28); io_wait(); //slave vector offset  -> 0x28
    outb(0x21, 0x04); io_wait(); //tell master slave is at IRQ2
    outb(0xA1, 0x02); io_wait(); //tell slave its cascade identity
    outb(0x21, 0x01); io_wait(); //8086 mode
    outb(0xA1, 0x01); io_wait();
    outb(0x21, 0x00);            //unmask all on master
    outb(0xA1, 0x00);            //unmask all on slave
}

void isr_install(void) {
    set_idt_gate(0,  (u32)isr0);  set_idt_gate(1,  (u32)isr1);
    set_idt_gate(2,  (u32)isr2);  set_idt_gate(3,  (u32)isr3);
    set_idt_gate(4,  (u32)isr4);  set_idt_gate(5,  (u32)isr5);
    set_idt_gate(6,  (u32)isr6);  set_idt_gate(7,  (u32)isr7);
    set_idt_gate(8,  (u32)isr8);  set_idt_gate(9,  (u32)isr9);
    set_idt_gate(10, (u32)isr10); set_idt_gate(11, (u32)isr11);
    set_idt_gate(12, (u32)isr12); set_idt_gate(13, (u32)isr13);
    set_idt_gate(14, (u32)isr14); set_idt_gate(15, (u32)isr15);
    set_idt_gate(16, (u32)isr16); set_idt_gate(17, (u32)isr17);
    set_idt_gate(18, (u32)isr18); set_idt_gate(19, (u32)isr19);
    set_idt_gate(20, (u32)isr20); set_idt_gate(21, (u32)isr21);
    set_idt_gate(22, (u32)isr22); set_idt_gate(23, (u32)isr23);
    set_idt_gate(24, (u32)isr24); set_idt_gate(25, (u32)isr25);
    set_idt_gate(26, (u32)isr26); set_idt_gate(27, (u32)isr27);
    set_idt_gate(28, (u32)isr28); set_idt_gate(29, (u32)isr29);
    set_idt_gate(30, (u32)isr30); set_idt_gate(31, (u32)isr31);

    pic_remap();

    set_idt_gate(32, (u32)irq0);  set_idt_gate(33, (u32)irq1);
    set_idt_gate(34, (u32)irq2);  set_idt_gate(35, (u32)irq3);
    set_idt_gate(36, (u32)irq4);  set_idt_gate(37, (u32)irq5);
    set_idt_gate(38, (u32)irq6);  set_idt_gate(39, (u32)irq7);
    set_idt_gate(40, (u32)irq8);  set_idt_gate(41, (u32)irq9);
    set_idt_gate(42, (u32)irq10); set_idt_gate(43, (u32)irq11);
    set_idt_gate(44, (u32)irq12); set_idt_gate(45, (u32)irq13);
    set_idt_gate(46, (u32)irq14); set_idt_gate(47, (u32)irq15);

    load_idt();
}

//unrecoverable CPU exceptions: report and halt
void isr_handler(registers_t *r) {
    set_color(VGA_COLOR(VGA_WHITE, VGA_RED));
    kprint("\n[EXCEPTION] ");
    kprint(exception_messages[r->int_no]);
    kprint(" (err=");
    kprint_hex(r->err_code);
    kprint(" eip=");
    kprint_hex(r->eip);
    if (r->int_no == 14) {
        //page fault: CR2 holds the faulting linear address
        u32 cr2;
        __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));
        kprint(" cr2=");
        kprint_hex(cr2);
    }
    kprint(")\n");
    set_color(VGA_COLOR(VGA_LIGHT_GREY, VGA_BLACK));
    for (;;) __asm__ volatile ("cli; hlt");
}

void irq_handler(registers_t *r) {
    //acknowledge the PIC(s) before running the handler
    if (r->int_no >= 40) outb(0xA0, 0x20); //EOI to slave
    outb(0x20, 0x20);                       //EOI to master

    if (interrupt_handlers[r->int_no])
        interrupt_handlers[r->int_no](r);
}

void register_interrupt_handler(u8 n, isr_t handler) {
    interrupt_handlers[n] = handler;
}
