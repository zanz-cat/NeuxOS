include ../../common.mk

.PHONY: all clean

CFLAGS += -I$(ROOTDIR)/include

all: libneuxos.a

$(OBJS): %.o:%.c
	$(CC) -c $(CFLAGS) -o $@ $^

$(ASMOBJS): %_S.o:%.S
	nasm $(ASMFLAGS) -f elf -o $@ $^

libneuxos.a: $(ASMOBJS) $(OBJS)
	ar cr $@ $^

clean:
	rm -f $(ASMOBJS) $(OBJS) libneuxos.a