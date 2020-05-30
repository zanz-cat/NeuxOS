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

interrupt_a.o: kernel/interrupt.asm
	nasm -f elf -o $@ $<

interrupt.o: kernel/interrupt.c include/type.h
	gcc -c $(CFLAGS) -o $@ $<

i8259a.o: kernel/i8259a.c include/const.h include/type.h
	gcc -c $(CFLAGS) -o $@ $<

kernel_a.o: kernel/kernel.asm
	nasm -f elf -o $@ $<

kernel.o: kernel/kernel.c include/const.h include/string.h \
 include/type.h include/stdio.h include/unistd.h
	gcc -c $(CFLAGS) -o $@ $<

string.o: lib/string.asm
	nasm -f elf -o $@ $<

stdio_a.o: lib/stdio.asm
	nasm -f elf -o $@ $<

stdio.o: lib/stdio.c include/type.h
	gcc -c $(CFLAGS) -o $@ $<

unistd_a.o: lib/unistd.asm
	nasm -f elf -o $@ $<

unistd.o: lib/unistd.c include/type.h
	gcc -c $(CFLAGS) -o $@ $<

app.o: app/app.c include/stdio.h include/type.h
	gcc -c $(CFLAGS) -o $@ $<

clock.o: kernel/clock.c include/stdio.h \
 include/type.h include/schedule.h
	gcc -c $(CFLAGS) -o $@ $<

schedule.o: kernel/schedule.c include/const.h \
 include/stdio.h include/type.h include/proc.h
	gcc -c $(CFLAGS) -o $@ $<

gdt.o: kernel/gdt.c include/type.h \
 include/protect.h include/type.h
	gcc -c $(CFLAGS) -o $@ $<

log.o: lib/log.c include/log.h include/stdio.h \
 include/type.h
	gcc -c $(CFLAGS) -o $@ $<

kernel.elf: kernel_a.o kernel.o string.o i8259a.o stdio_a.o \
 stdio.o unistd_a.o unistd.o interrupt_a.o interrupt.o clock.o \
 schedule.o gdt.o log.o app.o
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