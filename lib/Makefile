OBJS:=$(patsubst %.c, %.o, $(wildcard *.c))
ASMOBJS:=$(patsubst %.S, %_S.o, $(wildcard *.S))

.PHONY: all clean

all: $(ASMOBJS) $(OBJS)

$(ASMOBJS): %_S.o:%.S
	nasm $(ASMFLAGS) -f elf -o $@ $<

$(OBJS): %.o:%.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f *.o