include ../common.mk
include ../.config

ifeq ($(CONFIG_BOOT),fd)
KERNEL_ADDR=0x20000 # 128KB
else
KERNEL_ADDR=0x100000 # 1MB
endif

.PHONY: all clean

all: $(ASMOBJS) $(OBJS) kernel.elf

$(OBJS): %.o:%.c
	$(CC) -c $(CFLAGS) -o $@ $^

$(ASMOBJS): %_S.o:%.S
	nasm $(ASMFLAGS) -f elf -o $@ $^

kernel.elf: $(OBJS) $(ASMOBJS) $(DRIVEROBJS) $(FSOBJS) $(LIBOBJS)
	$(LD) $(LDFLAGS) -Ttext $(KERNEL_ADDR) -o $@ $^
	objcopy --remove-section=.note.gnu.property $@ 2>/dev/null

clean:
	rm -f $(ASMOBJS) $(OBJS) kernel.elf