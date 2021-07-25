SOURCE:=$(wildcard *.c)
ASMSOURCE:=kernel.S interrupt.S syscall.S
OBJS:=$(patsubst %.c, %.o, $(SOURCE))
ASMOBJS:=$(patsubst %.S, %_S.o, $(ASMSOURCE))

.PHONY: all clean

all: $(ASMOBJS) $(OBJS) kernel.elf

$(OBJS): %.o:%.c
	$(CC) -c $(CFLAGS) -o $@ $<

$(ASMOBJS): %_S.o:%.S
	nasm $(ASMFLAGS) -f elf -o $@ $<

kernel.elf: $(wildcard $(SRCDIR)/lib/*.o) $(OBJS) $(ASMOBJS) ../app/*.o
	$(LD) -m elf_i386 -Ttext 0x30700 -o $@ $^
	objcopy --remove-section=.note.gnu.property $@

clean:
	rm -f *.o kernel.elf