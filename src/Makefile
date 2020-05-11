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
	gcc -c -m32 -fno-builtin -Iinclude -fno-leading-underscore -o $@ $<

i8259a.o: kernel/i8259a.c include/const.h include/type.h
	gcc -c -m32 -fno-builtin -Iinclude -fno-leading-underscore -o $@ $<

kernel_a.o: kernel/kernel.asm
	nasm -f elf -o $@ $<

kernel.o: kernel/kernel.c include/const.h include/string.h \
 include/type.h include/stdio.h include/unistd.h
	gcc -c -m32 -fno-builtin -Iinclude -fno-leading-underscore -o $@ $<

string.o: lib/string.asm
	nasm -f elf -o $@ $<

stdio_a.o: lib/stdio.asm
	nasm -f elf -o $@ $<

stdio.o: lib/stdio.c include/type.h
	gcc -c -m32 -fno-builtin -Iinclude -fno-leading-underscore -o $@ $<

unistd_a.o: lib/unistd.asm
	nasm -f elf -o $@ $<

unistd.o: lib/unistd.c include/type.h
	gcc -c -m32 -fno-builtin -Iinclude -fno-leading-underscore -o $@ $<

kernel.elf: kernel_a.o kernel.o string.o i8259a.o stdio_a.o \
 stdio.o unistd_a.o unistd.o interrupt_a.o interrupt.o
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

clean:
	rm -f a.img kernel.elf *.o *.bin