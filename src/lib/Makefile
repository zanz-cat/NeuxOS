OBJS:=$(patsubst %.c, %.o, $(wildcard *.c))
ASMOBJS:=$(patsubst %.S, %_asm.o, $(wildcard *.S))

.PHONY: all clean

all: $(ASMOBJS) $(OBJS)

$(ASMOBJS): %_asm.o:%.S
	nasm $(ASMFLAGS) -f elf -o $@ $<

$(OBJS): %.o:%.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f *.o