#include <kernel/const.h>
#include <kernel/i8259a.h>
#include <kernel/sched.h>
#include <lib/print.h>
#include <lib/log.h>
#include <kernel/interrupt.h>

extern void app1();
extern void app2();
extern void kapp1();
extern void kapp2();

void *irq_handler_table[15];

static u8 idt_ptr[6];
struct gate idt[IDT_SIZE];
static char *int_err_msg[] = {
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

static void init_int_desc(u8 vector, u8 desc_type, int_handler handler, u8 privilege) 
{
    struct gate *int_desc = idt + vector;
    int_desc->selector = SELECTOR_KERNEL_CS;
    int_desc->dcount = 0;
    int_desc->attr = desc_type | (privilege << 5);
    int_desc->offset_high = ((u32)handler >> 16) & 0xffff;
    int_desc->offset_low = (u32)handler & 0xffff;
}

void exception_handler(int vec_no, int err_code, int eip, int cs, int eflags) 
{
    int color = current_console->color;
    current_console->color = 0x74; /* 灰底红字 */
    fprintk(current_console, 
            "\nKernel crashed!\n"
            "%s\n"
            "EFLAGS: 0x%x\n"
            "CS: 0x%x\n"
            "EIP: 0x%x\n", 
            int_err_msg[vec_no], eflags, cs, eip);

    if(err_code != 0xffffffff) {
        fprintk(current_console, "Error code: 0x%x\n", err_code);
    }
    current_console->color = color;
    asm("hlt");
}

static void serial2_handler() 
{
    putsk("serial2_handler\n");
}

static void serial1_handler() 
{
    putsk("serial1_handler\n");
}

static void lpt2_handler() 
{
    putsk("lpt2_handler\n");
}

static void floppy_handler() 
{
    putsk("floppy_handler\n");
}

static void lpt1_handler() 
{
    putsk("lpt1_handler\n");
}

static void real_clock_handler() 
{
    putsk("real_clock_handler\n");
}

static void mouse_handler() 
{
    putsk("mouse_handler\n");
}

static void copr_handler() 
{
    putsk("copr_handler\n");
}

static void harddisk_handler() 
{
    putsk("harddisk_handler\n");
}

void put_irq_handler(int vector, int_handler h) 
{
    irq_handler_table[vector] = h;
}

void enable_irq(int vector) 
{
    int port = vector < 8 ? INT_M_CTLMASK : INT_S_CTLMASK;
    u8 mask = in_byte(port);
    out_byte(port, mask & (~(0x1 << vector)));
}

void disable_irq(int vector) 
{
    int port = vector < 8 ? INT_M_CTLMASK : INT_S_CTLMASK;
    u8 mask = in_byte(port);
    out_byte(port, mask & (0x1 << vector));
}

void init_interrupt() 
{    
    log_info("init interrupt controller\n");
    init_8259a();
    
    log_info("init interrupt descriptor table\n");
    // init system int vector
    init_int_desc(INT_VECTOR_DIVIDE, DA_386IGate, divide_error, PRIVILEGE_KRNL);
    init_int_desc(INT_VECTOR_DEBUG, DA_386IGate, single_step_exception, PRIVILEGE_KRNL);
    init_int_desc(INT_VECTOR_NMI, DA_386IGate, nmi, PRIVILEGE_KRNL);
    init_int_desc(INT_VECTOR_BREAKPOINT, DA_386IGate, breakpoint_exception, PRIVILEGE_USER);
    init_int_desc(INT_VECTOR_OVERFLOW, DA_386IGate, overflow, PRIVILEGE_USER);
    init_int_desc(INT_VECTOR_BOUNDS, DA_386IGate, bounds_check, PRIVILEGE_KRNL);
    init_int_desc(INT_VECTOR_INVAL_OP, DA_386IGate, inval_opcode, PRIVILEGE_KRNL);
    init_int_desc(INT_VECTOR_COPROC_NOT, DA_386IGate, copr_not_available, PRIVILEGE_KRNL);
    init_int_desc(INT_VECTOR_DOUBLE_FAULT, DA_386IGate, double_fault, PRIVILEGE_KRNL);
    init_int_desc(INT_VECTOR_COPROC_SEG, DA_386IGate, copr_seg_overrun, PRIVILEGE_KRNL);
    init_int_desc(INT_VECTOR_INVAL_TSS, DA_386IGate, inval_tss, PRIVILEGE_KRNL);
    init_int_desc(INT_VECTOR_SEG_NOT, DA_386IGate, segment_not_present, PRIVILEGE_KRNL);
    init_int_desc(INT_VECTOR_STACK_FAULT, DA_386IGate, stack_exception, PRIVILEGE_KRNL);
    init_int_desc(INT_VECTOR_PROTECTION, DA_386IGate, general_protection, PRIVILEGE_KRNL);
    init_int_desc(INT_VECTOR_PAGE_FAULT, DA_386IGate, page_fault, PRIVILEGE_KRNL);
    init_int_desc(INT_VECTOR_COPROC_ERR, DA_386IGate, copr_error, PRIVILEGE_KRNL);
    init_int_desc(INT_VECTOR_SYSCALL, DA_386IGate, syscall, PRIVILEGE_USER);

    // init hardware int vector
    init_int_desc(INT_VECTOR_IRQ0 + 0, DA_386IGate, hwint00, PRIVILEGE_KRNL);
    init_int_desc(INT_VECTOR_IRQ0 + 1, DA_386IGate, hwint01, PRIVILEGE_KRNL);
    init_int_desc(INT_VECTOR_IRQ0 + 3, DA_386IGate, hwint03, PRIVILEGE_KRNL);
    init_int_desc(INT_VECTOR_IRQ0 + 4, DA_386IGate, hwint04, PRIVILEGE_KRNL);
    init_int_desc(INT_VECTOR_IRQ0 + 5, DA_386IGate, hwint05, PRIVILEGE_KRNL);
    init_int_desc(INT_VECTOR_IRQ0 + 6, DA_386IGate, hwint06, PRIVILEGE_KRNL);
    init_int_desc(INT_VECTOR_IRQ0 + 7, DA_386IGate, hwint07, PRIVILEGE_KRNL);
    init_int_desc(INT_VECTOR_IRQ8 + 0, DA_386IGate, hwint08, PRIVILEGE_KRNL);
    init_int_desc(INT_VECTOR_IRQ8 + 4, DA_386IGate, hwint12, PRIVILEGE_KRNL);
    init_int_desc(INT_VECTOR_IRQ8 + 5, DA_386IGate, hwint13, PRIVILEGE_KRNL);
    init_int_desc(INT_VECTOR_IRQ8 + 6, DA_386IGate, hwint14, PRIVILEGE_KRNL);

    // install hardware int handlers
    irq_handler_table[IRQ_SERIAL2] = serial2_handler;
    irq_handler_table[IRQ_SERIAL1] = serial1_handler;
    irq_handler_table[IRQ_LPT2] = lpt2_handler;
    irq_handler_table[IRQ_FLOPPY] = floppy_handler;
    irq_handler_table[IRQ_LPT1] = lpt1_handler;
    irq_handler_table[IRQ_REAL_CLOCK] = real_clock_handler;
    irq_handler_table[IRQ_MOUSE] = mouse_handler;
    irq_handler_table[IRQ_COPR] = copr_handler;
    irq_handler_table[IRQ_HARDDISK] = harddisk_handler;

    // init idt ptr
    u16 *p_idt_limit = (u16*)idt_ptr;
    u32 *p_idt_base = (u32*)(idt_ptr+2);
    *p_idt_limit = IDT_SIZE * sizeof(struct gate) - 1;
    *p_idt_base = (u32)&idt;

    asm("lidt %0"::"m"(idt_ptr):);
}