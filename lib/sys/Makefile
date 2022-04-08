include ../../common.mk

.PHONY: all clean

CFLAGS += -I$(ROOTDIR)/include

all: libsys.a

$(OBJS): %.o:%.c
	$(CC) -c $(CFLAGS) -o $@ $^

$(ASMOBJS): %_S.o:%.S
	nasm $(ASMFLAGS) -f elf -o $@ $^

libsys.a: $(ASMOBJS) $(OBJS)
	ar cr $@ $^

clean:
	rm -f $(ASMOBJS) $(OBJS) libsys.a