#include <drivers/io.h>
#include <drivers/i8259a.h>

#include <kernel/sched.h>
#include <kernel/interrupt.h>
#include <kernel/log.h>

#include "keymap.h"
#include "keyboard.h"

static struct keyboard_buf kb_buf;
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
    uint8_t scan_code = in_byte(KEYBOARD_IO_PORT);

    if (kb_buf.count >= KEYBOARD_IN_BYTES) {
        return;
    }
    *(kb_buf.head) = scan_code;
    kb_buf.head++;
    if (kb_buf.head == kb_buf.data + KEYBOARD_IN_BYTES) {
        kb_buf.head = kb_buf.data;
    }
    kb_buf.count++;
}

static uint8_t get_byte_from_kbuf()
{
    while (kb_buf.count <= 0) {
        yield();
    }

    disable_irq_n(IRQ_KEYBOARD);
    uint8_t scan_code = *(kb_buf.tail);
    kb_buf.tail++;
    if (kb_buf.tail == kb_buf.data + KEYBOARD_IN_BYTES) {
        kb_buf.tail = kb_buf.data;
    }

    kb_buf.count--;
    enable_irq_n(IRQ_KEYBOARD);

    return scan_code;
}

static void keyboard_wait()
{
    uint8_t kb_stat;
    do {
        kb_stat = in_byte(KB_CMD);
    } while (kb_stat & 0x02);
}

static void keyboard_ack()
{
    uint8_t kb_read;
    do {
        kb_read = in_byte(KB_DATA);
    } while (kb_read != KB_ACK);
}

static void set_leds()
{
    uint8_t leds = (caps_lock << 2) | (num_lock << 1) | scroll_lock;

    keyboard_wait();
    out_byte(KB_DATA, LED_CODE);
    keyboard_ack();

    keyboard_wait();
    out_byte(KB_DATA, leds);
    keyboard_ack();
}

void keyboard_read(void (*handler)(int, uint16_t), int fd)
{
    uint8_t scan_code;
    int is_make;
    uint16_t key = 0;
    uint32_t *keyrow;
    int code_with_E0 = 0;

    scan_code = get_byte_from_kbuf();
    if (scan_code == 0xe1) {
        uint8_t pausebrk_scode[] = {0xe1, 0x1d, 0x45,
                                0xe1, 0x9d, 0xc5};
        int is_pausebrk = 1;
        for (int i = 1; i < 6; i++) {
            if (get_byte_from_kbuf() != pausebrk_scode[i]) {
                is_pausebrk = 0;
                break;
            }
        }

        if(is_pausebrk) {
            key = PAUSEBREAK;
        }

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
        if (key == 0) {
            code_with_E0 = 1;
        }
    }
    if (key == PAUSEBREAK || key == PRINTSCREEN) {
        return;
    }

    is_make = (scan_code & FLAG_BREAK) ? 0 : 1;
    keyrow = &keymap[(scan_code & 0x7f) * MAP_COLS];
    column = 0;

    int caps = shift_l || shift_r;
    if (caps_lock && keyrow[0] >= 'a' && keyrow[0] <= 'z') {
        caps = !caps;
    }

    if (caps) {
        column = 1;
    }

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
    case CAPS_LOCK:
        if (is_make) {
            caps_lock = !caps_lock;
            set_leds();
        }
        break;
    case NUM_LOCK:
        if (is_make) {
            num_lock = !num_lock;
            set_leds();
        }
        break;
    case SCROLL_LOCK:
        if (is_make) {
            scroll_lock = !scroll_lock;
            set_leds();
        }
        break;
    default:
        break;
    }

    if (!is_make) {
        return;
    }

    if (key >= PAD_SLASH && key <= PAD_9) {
        switch (key) {
        case PAD_SLASH:
            key = '/';
            break;
        case PAD_STAR:
            key = '*';
            break;
        case PAD_MINUS:
            key = '-';
            break;
        case PAD_PLUS:
            key = '+';
            break;
        case PAD_ENTER:
            key = ENTER;
            break;
        default:
            if (num_lock && key >= PAD_0 && key <= PAD_9) {
                key = key - PAD_0 + '0';
            }
            else if (num_lock && key == PAD_DOT) {
                key = '.';
            }
            else{
                switch(key) {
                case PAD_HOME:
                    key = HOME;
                    break;
                case PAD_END:
                    key = END;
                    break;
                case PAD_PAGEUP:
                    key = PAGEUP;
                    break;
                case PAD_PAGEDOWN:
                    key = PAGEDOWN;
                    break;
                case PAD_INS:
                    key = INSERT;
                    break;
                case PAD_UP:
                    key = UP;
                    break;
                case PAD_DOWN:
                    key = DOWN;
                    break;
                case PAD_LEFT:
                    key = LEFT;
                    break;
                case PAD_RIGHT:
                    key = RIGHT;
                    break;
                case PAD_DOT:
                    key = DELETE;
                    break;
                default:
                    break;
                }
            }
            break;
        }
    }

    key |= shift_l ? FLAG_SHIFT_L : 0;
    key |= shift_r ? FLAG_SHIFT_R : 0;
    key |= ctrl_l ? FLAG_CTRL_L : 0;
    key |= ctrl_r ? FLAG_CTRL_R : 0;
    key |= alt_l ? FLAG_ALT_L : 0;
    key |= alt_r ? FLAG_ALT_R : 0;

    handler(fd, key);
}

void keyboard_setup()
{
    log_info("setup keyboard\n");

    kb_buf.count = 0;
    kb_buf.head = kb_buf.tail = kb_buf.data;

    shift_l = 0;
    shift_r = 0;
    ctrl_l = 0;
    ctrl_r = 0;
    alt_l = 0;
    alt_r = 0;

    caps_lock = 0;
    num_lock = 1;
    scroll_lock = 0;
    set_leds();

    irq_register_handler(IRQ_KEYBOARD, keyboard_handler);

    /* 开启键盘中断 */
    enable_irq_n(IRQ_KEYBOARD);
}