##################################################
# Makefile of pmtestx.asm (x=[1,2,3...])
##################################################

SRC:=kernel.asm
BIN:=$(subst .asm,.com,$(SRC))

.PHONY : everything

everything : $(BIN)
	sudo mount -o loop freedos.img /mnt/floppy/
	sudo mkdir -p /mnt/floppy/LeeOS
	sudo cp $(BIN) /mnt/floppy/LeeOS -v
	sudo umount /mnt/floppy/

$(BIN) : $(SRC)
	nasm $< -o $@

