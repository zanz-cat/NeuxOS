##################################################
# Makefile of pmtestx.asm (x=[1,2,3...])
##################################################

SRC:=boot.asm loader.asm
BIN:=$(subst .asm,.bin,$(SRC))

.PHONY : everything

everything : img $(BIN) kernel 
	dd if=boot.bin of=a.img bs=512 count=1 conv=notrunc
	sudo mount -o loop a.img /mnt/floppy
	sudo cp loader.bin /mnt/floppy/LOADER.BIN
	sudo cp kernel.bin /mnt/floppy/KERNEL.BIN
	sudo umount /mnt/floppy

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
	sudo mount -o loop a.img /mnt/floppy

umount:
	sudo umount /mnt/floppy
