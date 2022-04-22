include ../vars.mk
include ../.config

.PHONY: all clean

all: $(ASMOBJS) $(OBJS) kernel.elf

$(OBJS): %.o:%.c
	$(CC) -c $(CFLAGS) -o $@ $^

$(ASMOBJS): %.o:%.S
	nasm $(ASMFLAGS) -f elf -o $@ $^

kernel.elf: $(OBJS) $(ASMOBJS) $(DRIVEROBJS) $(FSOBJS) $(LIBOBJS) $(MMOBJS) $(ARCHOBJS)
	$(LD) $(LDFLAGS) -Ttext $(CONFIG_KERNEL_IMG_VM_ADDR) -o $@ $^
	objcopy --remove-section=.note.gnu.property $@ 2>/dev/null

clean:
	rm -f $(ASMOBJS) $(OBJS) kernel.elf