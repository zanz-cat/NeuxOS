#include "type.h"
#include "const.h"
#include "protect.h"
#include "stdio.h"
#include "i8259a.h"
#include "schedule.h"
#include "clock.h"

typedef void (*int_handler)();

void divide_error();
void single_step_exception();
void nmi();
void breakpoint_exception();
void overflow();
void bounds_check();
void inval_opcode();
void copr_not_available();
void double_fault();
void copr_seg_overrun();
void inval_tss();
void segment_not_present();
void stack_exception();
void general_protection();
void page_fault();
void copr_error();
void syscall();

void hwint00();
void hwint01();
void hwint03();
void hwint04();
void hwint05();
void hwint06();
void hwint07();
void hwint08();
void hwint12();
void hwint13();
void hwint14();

extern void app1();
extern void app2();
extern void kapp1();
extern void kapp2();

u8 idt_ptr[6];
GATE idt[IDT_SIZE];
void *irq_handler_table[15];

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

static void init_int_desc(u8 vector, u8 desc_type, int_handler handler, u8 privilege) {
    GATE *int_desc = idt + vector;
    int_desc->selector = SELECTOR_KERNEL_CS;
    int_desc->dcount = 0;
    int_desc->attr = desc_type | (privilege << 5);
    int_desc->offset_high = ((u32)handler >> 16) & 0xffff;
    int_desc->offset_low = (u32)handler & 0xffff;
}


void exception_handler(int vec_no, int err_code, int eip, int cs, int eflags) {
    int color = get_text_color();
    set_text_color(0x74); /* 灰底红字 */
    printf("\nException! --> \n");
    printf("%s\n", int_err_msg[vec_no]);
    printf("EFLAGS: 0x%x\n", eflags);
    printf("CS: 0x%x\n", cs);
    printf("EIP: 0x%x\n", eip);

    if(err_code != 0xffffffff){
        printf("Error code: 0x%x\n", err_code);
    }
    set_text_color(color);
}

static int f = 0;
static int a = 0;
void keyboard_handler() {
    f = !f;
    u8 code = in_byte(0x60);
    if (f) {
        return;
    }
    a %= 8;
    switch (a)
    {
    case 0:
        create_proc(app1);
        break;
    case 1:
        create_proc(app2);
        break;
    case 2:
        create_kproc(kapp1);
        break;
    case 3:
        create_kproc(kapp2);
        break;
    case 4:
        destroy_proc();
        break;
    case 5:
        destroy_proc();
        break;
    case 6:
        destroy_proc();
        break;
    case 7:
        destroy_proc();
        break;
    default:
        break;
    }

    a++;
}

void serial2_handler() {
    puts("serial2_handler\n");
}

void serial1_handler() {
    puts("serial1_handler\n");
}

void lpt2_handler() {
    puts("lpt2_handler\n");
}

void floppy_handler() {
    puts("floppy_handler\n");
}

void lpt1_handler() {
    puts("lpt1_handler\n");
}

void real_clock_handler() {
    puts("real_clock_handler\n");
}

void mouse_handler() {
    puts("mouse_handler\n");
}

void copr_handler() {
    puts("copr_handler\n");
}

void harddisk_handler() {
    puts("harddisk_handler\n");
}

void init_interrupt() {    
    puts("Init 8259A Interrupt Controller\n");
    init_8259A();
    
    puts("Init IDT\n");
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
    init_int_desc(INT_VECTOR_SYSCALL, DA_386IGate, syscall, PRIVILEGE_KRNL);

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
    irq_handler_table[0] = clock_handler;
    irq_handler_table[1] = keyboard_handler;
    irq_handler_table[3] = serial2_handler;
    irq_handler_table[4] = serial1_handler;
    irq_handler_table[5] = lpt2_handler;
    irq_handler_table[6] = floppy_handler;
    irq_handler_table[7] = lpt1_handler;
    irq_handler_table[8] = real_clock_handler;
    irq_handler_table[12] = mouse_handler;
    irq_handler_table[13] = copr_handler;
    irq_handler_table[14] = harddisk_handler;

    // init idt ptr
    u16 *p_idt_limit = (u16*)idt_ptr;
    u32 *p_idt_base = (u32*)(idt_ptr+2);
    *p_idt_limit = IDT_SIZE * sizeof(GATE) - 1;
    *p_idt_base = (u32)&idt;
}
