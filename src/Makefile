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

BOOT_SRC:=$(wildcard boot/*.asm)
BOOT_BIN:=$(subst .asm,.bin,$(BOOT_SRC))

KERNLE_SRC:=$(wildcard kernel/*.asm)
KERNLE_OBJ:=$(subst .asm,.o,$(KERNLE_SRC))

KERNEL_C_SRC:=$(wildcard kernel/*.c)
KERNEL_C_OBJ:=$(subst .c,.o,$(KERNEL_C_SRC))

LIB_SRC:=$(wildcard lib/*.asm)
LIB_OBJ:=$(subst .asm,.o,$(LIB_SRC))

.PHONY: clean mount umount run debug

a.img: $(BOOT_BIN) kernel.elf
	rm -f a.img
	bximage -q -mode=create -fd=1.44M a.img
	dd if=boot/boot.bin of=a.img bs=512 count=1 conv=notrunc
	$(MOUNTIMG)
	$(SUDO) cp boot/loader.bin $(MOUNTPOINT)/LOADER.BIN
	$(SUDO) cp kernel.elf $(MOUNTPOINT)/KERNEL.ELF
	$(UMOUNTIMG)

$(BOOT_BIN): $(BOOT_SRC)
	nasm $(subst .bin,.asm,$@) -o $@

$(KERNLE_OBJ): $(KERNLE_SRC)
	nasm -f elf -o $@ $(subst .o,.asm,$@)

$(KERNEL_C_OBJ): $(KERNEL_C_SRC)
	gcc -c -m32 -Iinclude -fno-leading-underscore -o $@ $(subst .o,.c,$@)

$(LIB_OBJ): $(LIB_SRC)
	nasm -f elf -o $@ $(subst .o,.asm,$@)

kernel.elf: $(KERNLE_OBJ) $(KERNEL_C_OBJ) $(LIB_OBJ)
	ld -m elf_i386 -o $@ $(KERNLE_OBJ) $(KERNEL_C_OBJ) $(LIB_OBJ)

clean:
	rm -f a.img $(BOOT_BIN) $(KERNLE_OBJ) $(KERNEL_C_OBJ) $(LIB_OBJ)

mount:
	$(MOUNTIMG)

umount:
	$(UMOUNTIMG)

run: a.img
	bochs -q

debug: a.img
	bochsdbg -q
