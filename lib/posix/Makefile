include ../../common.mk

.PHONY: all clean

CFLAGS += -I$(ROOTDIR)/include

all: libposix.a

$(OBJS): %.o:%.c
	$(CC) -c $(CFLAGS) -o $@ $^

$(ASMOBJS): %_S.o:%.S
	nasm $(ASMFLAGS) -f elf -o $@ $^

libposix.a: $(ASMOBJS) $(OBJS)
	ar cr $@ $^

clean:
	rm -f $(ASMOBJS) $(OBJS) libposix.a