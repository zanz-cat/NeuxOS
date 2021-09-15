include ../../common.mk

.PHONY: all clean

CFLAGS += -I$(ROOTDIR)/include

all: libc.a

$(OBJS): %.o:%.c
	$(CC) -c $(CFLAGS) -o $@ $^

$(ASMOBJS): %_S.o:%.S
	nasm $(ASMFLAGS) -f elf -o $@ $^

libc.a: $(ASMOBJS) $(OBJS)
	ar cr $@ $^

clean:
	rm -f $(ASMOBJS) $(OBJS) libc.a