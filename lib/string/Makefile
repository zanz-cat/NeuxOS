include ../../vars.mk

.PHONY: all clean

CFLAGS += -I$(ROOTDIR)/include

all: $(ASMOBJS) $(OBJS)

$(OBJS): %.o:%.c
	$(CC) -c $(CFLAGS) -o $@ $^

$(ASMOBJS): %.o:%.S
	nasm $(ASMFLAGS) -f elf -o $@ $^

clean:
	rm -f $(ASMOBJS) $(OBJS)