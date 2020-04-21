##################################################
# Makefile of pmtestx.asm (x=[1,2,3...])
##################################################

SRC:=boot.asm loader.asm
BIN:=$(subst .asm,.bin,$(SRC))

ifeq ($(OS), Windows_NT)
		SUDO:=
		MOUNTPOINT:=A:
		MOUNTIMG:=imdisk.exe -a -m $(MOUNTPOINT) -o hd -f a.img
		UMOUNTIMG:=imdisk.exe -D -m $(MOUNTPOINT)
		BOCHS:=bochs.exe
		BOCHSDBG:=bochsdbg.exe
else
		SUDO:=sudo
		MOUNTPOINT:=/mnt/floppy
		MOUNTIMG:=$(SUDO) mount -o loop a.img $(MOUNTPOINT)
		UMOUNTIMG:=$(SUDO) umount $(MOUNTPOINT)
		BOCHS:=bochs
		BOCHSDBG:=bochsdbg
endif


.PHONY : everything

everything : img $(BIN) kernel 
	dd if=boot.bin of=a.img bs=512 count=1 conv=notrunc
	$(MOUNTIMG)
	$(SUDO) cp loader.bin $(MOUNTPOINT)/LOADER.BIN
	$(SUDO) cp kernel.bin $(MOUNTPOINT)/KERNEL.BIN
	$(UMOUNTIMG)

$(BIN) : $(SRC)
	nasm $(subst .bin,.asm,$@) -o $@

kernel: kernel.asm
	nasm -f elf kernel.asm -o kernel.o
	ld -m elf_i386 -s kernel.o -o kernel.bin

img:
	rm -f a.img
	bximage -q -mode=create -fd=1.44M a.img

clean:
	rm -f *.bin a.img *.o

mount:
	$(MOUNTIMG)

umount:
	$(UMOUNTIMG)

run:
	$(BOCHS) -q

debug:
	$(BOCHSDBG) -q
