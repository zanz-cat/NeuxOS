##################################################
# Makefile of LeeOS
##################################################

ifeq ($(OS), Windows_NT)
		SUDO:=
		MOUNTPOINT:=A:
		MOUNTIMG:=imdisk.exe -a -m $(MOUNTPOINT) -o hd -f a.img
		UMOUNTIMG:=imdisk.exe -d -m $(MOUNTPOINT)
else
		SUDO:=sudo
		MOUNTPOINT:=/mnt/floppy
		MOUNTIMG:=$(SUDO) mount -o loop a.img $(MOUNTPOINT)
		UMOUNTIMG:=$(SUDO) umount $(MOUNTPOINT)
endif

CFLAGS:=-m32 -g -Werror -O0 -Iinclude -fno-builtin -fno-leading-underscore

.PHONY: mount umount run debug jrun jdebug clean

a.img: boot.bin loader.bin kernel.elf
	rm -f a.img
	bximage -q -mode=create -fd=1.44M a.img > /dev/null
	dd if=boot.bin of=a.img bs=512 count=1 conv=notrunc
	$(MOUNTIMG)
	$(SUDO) cp loader.bin $(MOUNTPOINT)/LOADER.BIN
	$(SUDO) cp kernel.elf $(MOUNTPOINT)/KERNEL.ELF
	$(UMOUNTIMG)

boot.bin: boot/boot.asm boot/boot.inc boot/fat12hdr.inc
	nasm -o $@ $<

loader.bin: boot/loader.asm boot/loader.inc boot/boot.inc \
 boot/fat12hdr.inc
	nasm -o $@ $<

interrupt_a.o: kernel/interrupt.asm include/const.inc
	nasm -f elf -o $@ $<

interrupt.o: kernel/interrupt.c include/type.h \
 include/const.h include/protect.h include/stdio.h \
 include/i8259a.h include/sched.h include/proc.h
	gcc -c $(CFLAGS) -o $@ $<

i8259a.o: kernel/i8259a.c include/const.h \
 include/protect.h include/type.h include/stdio.h
	gcc -c $(CFLAGS) -o $@ $<

kernel_a.o: kernel/kernel.asm include/common.inc \
 include/const.inc
	nasm -f elf -o $@ $<

kernel.o: kernel/kernel.c include/const.h include/string.h \
 include/type.h include/stdio.h include/unistd.h \
 include/sched.h include/proc.h include/protect.h \
 include/log.h include/gdt.h
	gcc -c $(CFLAGS) -o $@ $<

string.o: lib/string.asm include/common.inc
	nasm -f elf -o $@ $<

stdio_a.o: lib/stdio.asm include/common.inc
	nasm -f elf -o $@ $<

stdio.o: lib/stdio.c include/type.h
	gcc -c $(CFLAGS) -o $@ $<

syscall_a.o: kernel/syscall.asm
	nasm -f elf -o $@ $<

syscall.o: kernel/syscall.c include/clock.h
	gcc -c $(CFLAGS) -o $@ $<

unistd.o: lib/unistd.c include/type.h
	gcc -c $(CFLAGS) -o $@ $<

app.o: app/app.c include/stdio.h include/type.h
	gcc -c $(CFLAGS) -o $@ $<

clock.o: kernel/clock.c include/type.h \
 include/stdio.h include/i8259a.h include/sched.h \
 include/proc.h include/protect.h include/unistd.h
	gcc -c $(CFLAGS) -o $@ $<

sched.o: kernel/sched.c include/const.h \
 include/stdio.h include/type.h include/proc.h \
 include/protect.h include/gdt.h include/log.h \
 include/string.h
	gcc -c $(CFLAGS) -o $@ $<

gdt.o: kernel/gdt.c include/type.h \
 include/protect.h include/string.h include/const.h
	gcc -c $(CFLAGS) -o $@ $<

log.o: lib/log.c include/log.h include/stdio.h \
 include/type.h
	gcc -c $(CFLAGS) -o $@ $<

kernel.elf: kernel_a.o kernel.o string.o i8259a.o stdio_a.o \
 stdio.o unistd.o interrupt_a.o interrupt.o clock.o \
 sched.o gdt.o log.o syscall.o syscall_a.o app.o
	ld -m elf_i386 -Ttext 0x30700 -o $@ $^

mount:
	$(MOUNTIMG)

umount:
	$(UMOUNTIMG)

run: a.img
	bochs -q || true

debug: a.img
	bochsdbg -q

jrun:
	bochs -q || true

jdebug:
	bochsdbg -q

gdb:
	bochsgdb -f bochsgdbrc -q || true

clean:
	rm -f a.img kernel.elf *.o *.bin