#include "keyboard.h"
#include "const.h"
#include "type.h"
#include "interrupt.h"
#include "log.h"
#include "stdio.h"
#include "keymap.h"
#include "tty.h"
#include "sched.h"

static struct keyboard_input_buffer buf_in;
static int shift_l;
static int shift_r;
static int alt_l;
static int alt_r;
static int ctrl_l;
static int ctrl_r;
static int caps_lock;
static int num_lock;
static int scroll_lock;
static int column;

static void keyboard_handler() 
{
    u8 scan_code = in_byte(KEYBOARD_IO_PORT);
    
    if (buf_in.count >= KEYBOARD_IN_BYTES)
        return;

    *(buf_in.head) = scan_code;
    buf_in.head++;
    if (buf_in.head == buf_in.data + KEYBOARD_IN_BYTES)
        buf_in.head = buf_in.data;

    buf_in.count++;
}

u8 get_byte_from_kbuf() 
{
    while (buf_in.count <= 0)
        yield();
    
    disable_irq(IRQ_KEYBOARD);
    u8 scan_code = *(buf_in.tail);
    buf_in.tail++;
    if (buf_in.tail == buf_in.data + KEYBOARD_IN_BYTES) 
        buf_in.tail = buf_in.data;

    buf_in.count--;
    enable_irq(IRQ_KEYBOARD);

    return scan_code;
}

void keyboard_read() 
{
    u8 scan_code;
    char output[2];
    int is_make;
    u32 key = 0;
    u32 *keyrow;
    int code_with_E0 = 0;

    scan_code = get_byte_from_kbuf();
    if (scan_code == 0xe1) {
        u8 pausebrk_scode[] = {0xe1, 0x1d, 0x45, 
                                0xe1, 0x9d, 0xc5};
        int is_pausebrk = 1;
        for (int i = 1; i<6; i++) {
            if (get_byte_from_kbuf() != pausebrk_scode[i]) {
                is_pausebrk = 0;
                break;
            }
        }

        if(is_pausebrk)
            key = PAUSEBREAK;

    } else if (scan_code == 0xe0) {
        scan_code = get_byte_from_kbuf();
        /* printscreen keydown */
        if (scan_code == 0x2a && get_byte_from_kbuf() == 0xe0 && get_byte_from_kbuf() == 0x37) {
            key = PRINTSCREEN;
            is_make = 1;
        }
        /* printscreen keyup */
        if (scan_code == 0xb7 && get_byte_from_kbuf() == 0xe0 && get_byte_from_kbuf() == 0xaa) {
            key = PRINTSCREEN;
            is_make = 0;
        }
        /* todo */
        if (key == 0) 
            code_with_E0 = 1;
    }

    if((key != PAUSEBREAK) && (key != PRINTSCREEN)) {
        is_make = (scan_code & FLAG_BREAK) ? 0 : 1;
        keyrow = &keymap[(scan_code & 0x7f) * MAP_COLS];
        column = 0;
        
        if (shift_l || shift_r) 
            column = 1;
        
        if (code_with_E0) {
            column = 2;
            code_with_E0 = 0;
        }

        key = keyrow[column];
        switch (key) {
        case SHIFT_L:
            shift_l = is_make;
            break;
        case SHIFT_R:
            shift_r = is_make;
            break;
        case CTRL_L:
            ctrl_l = is_make;
            break;
        case CTRL_R:
            ctrl_r = is_make;
            break;
        case ALT_L:
            alt_l = is_make;
            break;
        case ALT_R:
            alt_r = is_make;
            break;
        default:
            break;
        }

        if (is_make) {
            key |= shift_l ? FLAG_SHIFT_L : 0;
            key |= shift_r ? FLAG_SHIFT_R : 0;
            key |= ctrl_l ? FLAG_CTRL_L : 0;
            key |= ctrl_r ? FLAG_CTRL_R : 0;
            key |= alt_l ? FLAG_ALT_L : 0;
            key |= alt_r ? FLAG_ALT_R : 0;

            in_process(key);
        }
    }
}

void init_keyboard() 
{
    log_info("init keyboard\n");

    buf_in.count = 0;
    buf_in.head = buf_in.tail = buf_in.data;

    put_irq_handler(IRQ_KEYBOARD, keyboard_handler);

    /* 开启键盘中断 */
    enable_irq(IRQ_KEYBOARD);
}