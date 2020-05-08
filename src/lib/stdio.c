#include "type.h"
#define DEFAULT_TEXT_COLOR 0x7

u32 _putchar(u32 ch, u8 color);

u32 putchar(u32 ch){
    return _putchar(ch, DEFAULT_TEXT_COLOR);
}

u32 color_putchar(u32 ch, u8 color){
    return _putchar(ch, color);
}

u32 puts(const u8* str){
    int ret = 0;
    for(const u8* i = str; *i != '\0'; i++) {
        ret = _putchar(*i, DEFAULT_TEXT_COLOR);
        if(ret != *i) {
            return ret;
        }
    }
    
    return 0;
}

u32 color_puts(const u8* str, u8 color){
    int ret = 0;
    for(const u8* i = str; *i != '\0'; i++) {
        ret = _putchar(*i, color);
        if(ret != *i) {
            return ret;
        }
    }
    
    return 0;
}