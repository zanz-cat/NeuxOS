export EXTRA_CFLAGS=-DNO_PAGE
CURDIR:=$(patsubst %/,%, $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
include ../../common.mk
include ../../.config

.PHONY: all clean

ALLOBJS:=$(OBJS) $(DRIVERSDIR)/io_S.o monitor.o $(LIBOBJS)

all: monitor.o mbr.bin boot.bin loader.bin loader.elf

monitor.o:
	make -C $(ROOTDIR)/drivers/monitor OUTDIR=$(CURDIR)

mbr.bin: mbr.S
	nasm $(ASMFLAGS) -o $@ $^
	@test `stat -c "%s" $@` -gt 446 && \
	echo "$@ too large(`stat -c "%s" $@` > 446)" && \
	exit 1 || exit 0

boot.bin: boot.S
	nasm $(ASMFLAGS) -o $@ $^
	@test `stat -c "%s" $@` -gt $(CONFIG_EXT2_BS) && \
	echo "$@ too large(`stat -c "%s" $@` > $(CONFIG_EXT2_BS))" && \
	exit 1 || exit 0

.loader_S.bin: loader.S
	nasm $(ASMFLAGS) -o $@ $^

# loader.c text address in memory
.loader_C.text: .loader_S.bin
	@LoaderAddr=`cat ../const.S | grep LOADER_ADDR | awk '{print $$3}'` && \
	[ $${LoaderAddr} ] || echo "invalid LOADER_ADDR" && [ $${LoaderAddr} ] && \
	TextAddr=`expr $$(stat -c "%s" $<) + $$(printf "%d" $${LoaderAddr})` && \
	printf "0x%x" $${TextAddr} > $@ && echo "$@ is `cat $@`"

$(OBJS): %.o:%.c
	$(CC) -c $(CFLAGS) -o $@ $^

.loader.bin: .loader_C.text $(ALLOBJS)
	$(LD) $(LDFLAGS) -o $@ -Ttext $$(cat $<) --oformat binary $(ALLOBJS)

loader.elf: .loader_C.text $(ALLOBJS)
	$(LD) $(LDFLAGS) -o $@ -Ttext $$(cat $<) $(ALLOBJS)

loader.bin: .loader_S.bin .loader.bin
	cat $^ > $@
	@test `stat -c "%s" $@` -gt 65536 && \
	echo "loader too large(`stat -c "%s" $@` > 65536)" && \
	exit 1 || exit 0

clean:
	rm -f .loader_C.text .loader_S.bin $(OBJS) .loader.bin loader.bin loader.elf boot.bin mbr.bin
	make -C $(ROOTDIR)/drivers/monitor OUTDIR=$(CURDIR) EXTRA_CFLAGS=-DNO_PAGE clean