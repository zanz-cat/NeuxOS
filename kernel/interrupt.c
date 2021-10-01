#include <stddef.h>

#include <include/syscall.h>

#include "log.h"
#include "sched.h"
#include "printk.h"
#include "interrupt.h"

// define in interrupt.S
void irq_hwint00();
void irq_hwint01();
void irq_hwint03();
void irq_hwint04();
void irq_hwint05();
void irq_hwint06();
void irq_hwint07();
void irq_hwint08();
void irq_hwint12();
void irq_hwint13();
void irq_hwint14();
void irq_ex_divide_error();
void irq_ex_single_step();
void irq_ex_nmi();
void irq_ex_breakpoint();
void irq_ex_overflow();
void irq_ex_bounds_check();
void irq_ex_inval_opcode();
void irq_ex_copr_not_available();
void irq_ex_double_fault();
void irq_ex_copr_seg_overrun();
void irq_ex_inval_tss();
void irq_ex_segment_not_present();
void irq_ex_stack_exception();
void irq_ex_general_protection();
void irq_ex_page_fault();
void irq_ex_copr_error();
void irq_ex_syscall();

#define IDT_SIZE 256
/* 权限 */
#define PRIVILEGE_KRNL 0
#define PRIVILEGE_TASK 1
#define PRIVILEGE_USER 3

struct idtr {
    uint16_t limit;
    void *base;
} __attribute__((packed));

irq_handler irq_handlers[IRQ_COUNT];

static struct gate idt[IDT_SIZE];
static char *exception_msg[] = {
    "#DE Divide Error",
    "#DB RESERVED",
    "—  NMI Interrupt",
    "#BP Breakpoint",
    "#OF Overflow",
    "#BR BOUND Range Exceeded",
    "#UD Invalid Opcode (Undefined Opcode)",
    "#NM Device Not Available (No Math Coprocessor)",
    "#DF Double Fault",
    "    Coprocessor Segment Overrun (reserved)",
    "#TS Invalid TSS",
    "#NP Segment Not Present",
    "#SS Stack-Segment Fault",
    "#GP General Protection",
    "#PF Page Fault",
    "—  (Intel reserved. Do not use.)",
    "#MF x87 FPU Floating-Point Error (Math Fault)",
    "#AC Alignment Check",
    "#MC Machine Check",
    "#XF SIMD Floating-Point Exception"
};

static void init_int_desc(uint8_t vector, uint8_t desc_type, irq_handler handler, uint8_t privilege)
{
    struct gate *int_desc = &idt[vector];
    int_desc->selector = SELECTOR_KERNEL_CS;
    int_desc->dcount = 0;
    int_desc->attr = desc_type | (privilege << 5);
    int_desc->offset_high = ((uint32_t)handler >> 16) & 0xffff;
    int_desc->offset_low = (uint32_t)handler & 0xffff;
}

void generic_ex_handler(int vec_no, int err_code, int eip, int cs, int eflags)
{
    uint8_t color, color_temp;

    color_temp = 0x74; /* 灰底红字 */
    tty_color(tty_current, TTY_OP_GET, &color);
    tty_color(tty_current, TTY_OP_SET, &color_temp);
    fprintk(TTY0, "\nKernel crashed!\n%s\nTASK: %u(%s)\n"
           "EFLAGS: 0x%x\nCS: 0x%x\nEIP: 0x%x\n",
           exception_msg[vec_no], current->pid, 
           current->exe, eflags, cs, eip);

    if(err_code != 0xffffffff) {
        printk("Error code: 0x%x\n", err_code);
    }
    tty_color(tty_current, TTY_OP_SET, &color);
    asm("hlt");
}

static void serial2_handler()
{
    printk("serial2_handler\n");
}

static void serial1_handler()
{
    printk("serial1_handler\n");
}

static void lpt2_handler()
{
    printk("lpt2_handler\n");
}

static void floppy_handler()
{
    printk("floppy_handler\n");
}

static void lpt1_handler()
{
    printk("lpt1_handler\n");
}

static void real_clock_handler()
{
    printk("real_clock_handler\n");
}

static void mouse_handler()
{
    printk("mouse_handler\n");
}

static void copr_handler()
{
    printk("copr_handler\n");
}

void irq_register_handler(int vector, irq_handler h)
{
    irq_handlers[(vector - INT_VECTOR_IRQ0)] = h;
}

void irq_setup()
{
    struct idtr idtr;

    log_info("setup interrupt\n");
    setup_8259a();

    // initialize exception interrupt descriptor
    init_int_desc(IRQ_EX_DIVIDE, DA_386IGate, irq_ex_divide_error, PRIVILEGE_KRNL);
    init_int_desc(IRQ_EX_DEBUG, DA_386IGate, irq_ex_single_step, PRIVILEGE_KRNL);
    init_int_desc(IRQ_EX_NMI, DA_386IGate, irq_ex_nmi, PRIVILEGE_KRNL);
    init_int_desc(IRQ_EX_BREAKPOINT, DA_386IGate, irq_ex_breakpoint, PRIVILEGE_USER);
    init_int_desc(IRQ_EX_OVERFLOW, DA_386IGate, irq_ex_overflow, PRIVILEGE_USER);
    init_int_desc(IRQ_EX_BOUNDS, DA_386IGate, irq_ex_bounds_check, PRIVILEGE_KRNL);
    init_int_desc(IRQ_EX_INVAL_OP, DA_386IGate, irq_ex_inval_opcode, PRIVILEGE_KRNL);
    init_int_desc(IRQ_EX_COTASK_NOT, DA_386IGate, irq_ex_copr_not_available, PRIVILEGE_KRNL);
    init_int_desc(IRQ_EX_DOUBLE_FAULT, DA_386IGate, irq_ex_double_fault, PRIVILEGE_KRNL);
    init_int_desc(IRQ_EX_COTASK_SEG, DA_386IGate, irq_ex_copr_seg_overrun, PRIVILEGE_KRNL);
    init_int_desc(IRQ_EX_INVAL_TSS, DA_386IGate, irq_ex_inval_tss, PRIVILEGE_KRNL);
    init_int_desc(IRQ_EX_SEG_NOT, DA_386IGate, irq_ex_segment_not_present, PRIVILEGE_KRNL);
    init_int_desc(IRQ_EX_STACK_FAULT, DA_386IGate, irq_ex_stack_exception, PRIVILEGE_KRNL);
    init_int_desc(IRQ_EX_PROTECTION, DA_386IGate, irq_ex_general_protection, PRIVILEGE_KRNL);
    init_int_desc(IRQ_EX_PAGE_FAULT, DA_386IGate, irq_ex_page_fault, PRIVILEGE_KRNL);
    init_int_desc(IRQ_EX_COTASK_ERR, DA_386IGate, irq_ex_copr_error, PRIVILEGE_KRNL);
    init_int_desc(IRQ_EX_SYSCALL, DA_386IGate, irq_ex_syscall, PRIVILEGE_USER);

    // initialize hardware interrupt descriptor
    init_int_desc(IRQ_CLOCK, DA_386IGate, irq_hwint00, PRIVILEGE_KRNL);
    init_int_desc(IRQ_KEYBOARD, DA_386IGate, irq_hwint01, PRIVILEGE_KRNL);
    init_int_desc(IRQ_SERIAL2, DA_386IGate, irq_hwint03, PRIVILEGE_KRNL);
    init_int_desc(IRQ_SERIAL1, DA_386IGate, irq_hwint04, PRIVILEGE_KRNL);
    init_int_desc(IRQ_LPT2, DA_386IGate, irq_hwint05, PRIVILEGE_KRNL);
    init_int_desc(IRQ_FLOPPY, DA_386IGate, irq_hwint06, PRIVILEGE_KRNL);
    init_int_desc(IRQ_LPT1, DA_386IGate, irq_hwint07, PRIVILEGE_KRNL);
    init_int_desc(IRQ_REAL_CLOCK, DA_386IGate, irq_hwint08, PRIVILEGE_KRNL);
    init_int_desc(IRQ_MOUSE, DA_386IGate, irq_hwint12, PRIVILEGE_KRNL);
    init_int_desc(IRQ_COPR, DA_386IGate, irq_hwint13, PRIVILEGE_KRNL);
    init_int_desc(IRQ_HARDDISK, DA_386IGate, irq_hwint14, PRIVILEGE_KRNL);

    // install hardware int handlers
    irq_register_handler(IRQ_SERIAL2, serial2_handler);
    irq_register_handler(IRQ_SERIAL1, serial1_handler);
    irq_register_handler(IRQ_LPT2, lpt2_handler);
    irq_register_handler(IRQ_FLOPPY, floppy_handler);
    irq_register_handler(IRQ_LPT1, lpt1_handler);
    irq_register_handler(IRQ_REAL_CLOCK, real_clock_handler);
    irq_register_handler(IRQ_MOUSE, mouse_handler);
    irq_register_handler(IRQ_COPR, copr_handler);

    // init idt ptr
    idtr.base = &idt;
    idtr.limit = IDT_SIZE * sizeof(struct gate) - 1;

    asm("lidt %0"::"m"(idtr):);
}
