##################################################
# Makefile of pmtestx.asm (x=[1,2,3...])
##################################################

SRC:=boot.asm loader.asm
BIN:=$(subst .asm,.bin,$(SRC))

.PHONY : everything

everything : $(BIN) 
	dd if=boot.bin of=a.img bs=512 count=1 conv=notrunc
	#sudo mount -o loop a.img /mnt/floppy
	#sudo cp loader.bin /mnt/floppy/LOADER.BIN
	#sudo umount /mnt/floppy

$(BIN) : $(SRC)
	nasm $< -o $@

img:
	rm -f a.img
	bximage -q -mode=create -fd=1.44M a.img

clean:
	rm -f *.bin a.img
