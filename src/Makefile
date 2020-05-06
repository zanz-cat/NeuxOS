##################################################
# Makefile of pmtestx.asm (x=[1,2,3...])
##################################################

ifeq ($(OS), Windows_NT)
		SUDO:=
		MOUNTPOINT:=A:
		MOUNTIMG:=imdisk.exe -a -m $(MOUNTPOINT) -o hd -f a.img
		UMOUNTIMG:=imdisk.exe -D -m $(MOUNTPOINT)
else
		SUDO:=sudo
		MOUNTPOINT:=/mnt/floppy
		MOUNTIMG:=$(SUDO) mount -o loop a.img $(MOUNTPOINT)
		UMOUNTIMG:=$(SUDO) umount $(MOUNTPOINT)
endif

.PHONY: clean mount umount run debug

a.img: boot.bin loader.bin kernel.elf
	rm -f a.img
	bximage -q -mode=create -fd=1.44M a.img
	dd if=boot.bin of=a.img bs=512 count=1 conv=notrunc
	$(MOUNTIMG)
	$(SUDO) cp loader.bin $(MOUNTPOINT)/LOADER.BIN
	$(SUDO) cp kernel.elf $(MOUNTPOINT)/KERNEL.ELF
	$(UMOUNTIMG)

boot.bin: boot/boot.asm boot/boot.inc boot/fat12hdr.inc
	nasm -o $@ $<

loader.bin: boot/loader.asm boot/loader.inc boot/boot.inc boot/fat12hdr.inc
	nasm -o $@ $<

start.o: kernel/start.c /usr/include/stdc-predef.h include/const.h \
 include/string.h include/const.h include/type.h
	gcc -c -m32 -Iinclude -fno-leading-underscore -o $@ $<

i8259a.o: kernel/i8259a.c /usr/include/stdc-predef.h include/const.h \
 include/kliba.h include/type.h
	gcc -c -m32 -Iinclude -fno-leading-underscore -o $@ $<

kernel.o: kernel/kernel.asm
	nasm -f elf -o $@ $<

string.o: lib/string.asm
	nasm -f elf -o $@ $<

kliba.o: lib/kliba.asm
	nasm -f elf -o $@ $<

kernel.elf: kernel.o start.o string.o i8259a.o kliba.o
	ld -m elf_i386 -Ttext 0x30700 -o $@ $^

clean:
	rm -f a.img kernel.elf *.o *.bin

mount:
	$(MOUNTIMG)

umount:
	$(UMOUNTIMG)

run:
	bochs -q

debug:
	bochsdbg -q
