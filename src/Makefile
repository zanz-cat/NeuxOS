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

SRC:=boot.asm loader.asm
BIN:=$(subst .asm,.bin,$(SRC))

ELFSRC:=kernel.asm
ELF:=$(subst .asm,.elf,$(ELFSRC))

.PHONY : clean mount umount run debug

a.img: $(BIN) $(ELF)
	rm -f a.img
	bximage -q -mode=create -fd=1.44M a.img
	dd if=boot.bin of=a.img bs=512 count=1 conv=notrunc
	$(MOUNTIMG)
	$(SUDO) cp loader.bin $(MOUNTPOINT)/LOADER.BIN
	$(SUDO) cp kernel.elf $(MOUNTPOINT)/KERNEL.ELF
	$(UMOUNTIMG)

$(BIN) : $(SRC)
	nasm $(subst .bin,.asm,$@) -o $@

$(ELF) : $(SRC)
	nasm -f elf $(subst .elf,.asm,$@) -o $(subst .elf,.o,$@)
	ld -m elf_i386 -s $(subst .elf,.o,$@) -o $@

clean:
	rm -f *.o *.bin *.elf a.img

mount:
	$(MOUNTIMG)

umount:
	$(UMOUNTIMG)

run: a.img
	bochs -q

debug: a.img
	bochsdbg -q
