##################################################
# Makefile of NeuxOS
##################################################

OS?=$(shell uname)
CC:=gcc
LD:=ld
CFLAGS:=-m32 -g -Werror -O0 -Iinclude -fno-builtin -nostdinc -fno-leading-underscore
ASMFLAGS:=-Werror
BUILD_DIR=../build

ifeq ($(OS),Darwin)
        CC:=x86_64-elf-gcc
	LD:=x86_64-elf-ld
endif

$(shell test -d $(BUILD_DIR) || mkdir $(BUILD_DIR))
$(shell test -d $(BUILD_DIR)/floppy || mkdir $(BUILD_DIR)/floppy)

ifeq ($(OS), Windows_NT)
	SUDO:=
	MOUNTPOINT:=A:
	MOUNTIMG:=imdisk.exe -a -m $(MOUNTPOINT) -o hd -f $(BUILD_DIR)/a.img
	UMOUNTIMG:=imdisk.exe -d -m $(MOUNTPOINT)
endif
ifeq ($(OS),Darwin)
	SUDO:=
	MOUNTPOINT:=$(BUILD_DIR)/floppy
	MOUNTIMG:=mount -t msdos `hdiutil attach -imagekey diskimage-class=CRawDiskImage -nomount $(BUILD_DIR)/a.img` $(MOUNTPOINT)
	UMOUNTIMG:=hdiutil detach $(MOUNTPOINT)
endif
ifeq ($(OS),Linux)
	SUDO:=sudo
	MOUNTPOINT:=$(BUILD_DIR)/floppy
	MOUNTIMG:=$(SUDO) mount -o loop $(BUILD_DIR)/a.img $(MOUNTPOINT)
	UMOUNTIMG:=$(SUDO) umount $(MOUNTPOINT)
endif

.PHONY: mount umount run debug jrun jdebug clean

$(BUILD_DIR)/a.img: $(BUILD_DIR)/boot.bin $(BUILD_DIR)/loader.bin $(BUILD_DIR)/kernel.elf
	rm -f $(BUILD_DIR)/a.img
	bximage -q -mode=create -fd=1.44M $(BUILD_DIR)/a.img > /dev/null
	dd if=$(BUILD_DIR)/boot.bin of=$(BUILD_DIR)/a.img bs=512 count=1 conv=notrunc
	$(MOUNTIMG)
	$(SUDO) cp $(BUILD_DIR)/loader.bin $(MOUNTPOINT)/LOADER.BIN
	$(SUDO) cp $(BUILD_DIR)/kernel.elf $(MOUNTPOINT)/KERNEL.ELF
	$(UMOUNTIMG)

$(BUILD_DIR)/boot.bin: boot/boot.asm boot/boot.inc boot/fat12hdr.inc
	nasm $(ASMFLAGS) -o $@ $<

$(BUILD_DIR)/loader.bin: boot/loader.asm boot/loader.inc boot/boot.inc \
 boot/fat12hdr.inc
	nasm $(ASMFLAGS) -o $@ $<

$(BUILD_DIR)/interrupt_a.o: kernel/interrupt.asm include/const.inc
	nasm $(ASMFLAGS) -f elf -o $@ $<

$(BUILD_DIR)/kernel_a.o: kernel/kernel.asm include/common.inc \
 include/const.inc
	nasm $(ASMFLAGS) -f elf -o $@ $<

$(BUILD_DIR)/string.o: lib/string.asm include/common.inc
	nasm $(ASMFLAGS) -f elf -o $@ $<

$(BUILD_DIR)/syscall_a.o: kernel/syscall.asm
	nasm $(ASMFLAGS) -f elf -o $@ $<

$(BUILD_DIR)/i8259a.o: kernel/i8259a.c include/const.h include/protect.h \
 include/type.h
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR)/interrupt.o: kernel/interrupt.c include/interrupt.h include/const.h \
 include/print.h include/type.h include/tty.h include/i8259a.h \
 include/sched.h include/proc.h include/protect.h include/log.h
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR)/keyboard.o: kernel/keyboard.c include/keyboard.h include/const.h \
 include/interrupt.h include/log.h include/keymap.h include/type.h \
 include/keyboard.h include/sched.h include/proc.h include/protect.h \
 include/tty.h
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR)/clock.o: kernel/clock.c include/type.h include/print.h include/type.h \
 include/i8259a.h include/sched.h include/proc.h include/protect.h \
 include/unistd.h include/const.h include/log.h include/interrupt.h
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR)/sched.o: kernel/sched.c include/const.h include/print.h include/type.h \
 include/protect.h include/proc.h include/proc.h include/protect.h \
 include/gdt.h include/log.h include/string.h include/const.h
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR)/kernel.o: kernel/kernel.c include/const.h include/string.h \
 include/const.h include/type.h include/type.h include/print.h \
 include/unistd.h include/protect.h include/sched.h include/proc.h \
 include/protect.h include/log.h include/gdt.h include/clock.h \
 include/keyboard.h
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR)/gdt.o: kernel/gdt.c include/type.h include/protect.h include/type.h \
 include/string.h include/const.h include/log.h
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR)/syscall.o: kernel/syscall.c include/clock.h
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR)/app.o: app/app.c include/print.h include/type.h include/unistd.h
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR)/unistd.o: lib/unistd.c include/type.h include/const.h
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR)/print.o: kernel/print.c include/type.h
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR)/log.o: lib/log.c include/log.h include/print.h include/type.h
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR)/tty.o: kernel/tty.c include/keyboard.h
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR)/stdio.o: lib/stdio.c include/stdio.h
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR)/common.o: lib/common.c include/common.h
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR)/kernel.elf: $(BUILD_DIR)/kernel_a.o $(BUILD_DIR)/kernel.o \
 $(BUILD_DIR)/string.o $(BUILD_DIR)/i8259a.o $(BUILD_DIR)/tty.o \
 $(BUILD_DIR)/print.o $(BUILD_DIR)/unistd.o $(BUILD_DIR)/interrupt_a.o \
 $(BUILD_DIR)/interrupt.o $(BUILD_DIR)/clock.o  $(BUILD_DIR)/sched.o \
 $(BUILD_DIR)/gdt.o $(BUILD_DIR)/log.o $(BUILD_DIR)/syscall.o \
 $(BUILD_DIR)/syscall_a.o $(BUILD_DIR)/keyboard.o $(BUILD_DIR)/common.o \
 $(BUILD_DIR)/app.o $(BUILD_DIR)/stdio.o
	$(LD) -m elf_i386 -Ttext 0x30700 -o $@ $^

mount:
	$(MOUNTIMG)

umount:
	$(UMOUNTIMG)

run: $(BUILD_DIR)/a.img
	bochs -q || true

debug: $(BUILD_DIR)/a.img
	bochsdbg -q

jrun:
	bochs -q || true

jdebug:
	bochsdbg -q

gdb:
	bochsgdb -f bochsgdbrc -q || true

clean:
	rm -rf $(BUILD_DIR)
