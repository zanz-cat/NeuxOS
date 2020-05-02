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

kerneldeps=kernel lib
modules=boot $(kerneldeps)

.PHONY: clean mount umount run debug $(modules)

img: boot kernel.elf
	rm -f a.img
	bximage -q -mode=create -fd=1.44M a.img
	dd if=boot/boot.bin of=a.img bs=512 count=1 conv=notrunc
	$(MOUNTIMG)
	$(SUDO) cp boot/loader.bin $(MOUNTPOINT)/LOADER.BIN
	$(SUDO) cp kernel.elf $(MOUNTPOINT)/KERNEL.ELF
	$(UMOUNTIMG)

$(modules):
	make -C $@

kernel.elf: $(kerneldeps)
	ld -m elf_i386 -Ttext 0x30700 -o $@ $(foreach n,$(kerneldeps),$(n)/*.o)

clean:
	rm -f a.img kernel.elf
	for m in $(modules); do \
		make -C $$m clean; \
	done

mount:
	$(MOUNTIMG)

umount:
	$(UMOUNTIMG)

run:
	bochs -q

debug:
	bochsdbg -q
