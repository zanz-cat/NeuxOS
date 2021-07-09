##################################################
# Makefile of NeuxOS
##################################################

OS?=$(shell uname)
CC:=gcc
LD:=ld
SRCDIR:=$(patsubst %/, %, $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
CFLAGS:=-m32 -g -Werror -O0 -I$(SRCDIR)/include -fno-builtin -nostdinc -fno-leading-underscore
ASMFLAGS:=-Werror
BOOT?=hd
SUBDIRS:=app boot lib kernel
CLEAN_SUBDIRS:=$(addprefix _clean_,$(SUBDIRS))

ifeq ($(OS),Darwin)
	CC:=x86_64-elf-gcc
	LD:=x86_64-elf-ld
endif

ifeq ($(OS), Windows_NT)
	SUDO:=
	MOUNTPOINT:=A:
	MOUNTIMG:=imdisk.exe -a -m $(MOUNTPOINT) -o hd -f $(BOOT).img
	UMOUNTIMG:=imdisk.exe -d -m $(MOUNTPOINT)
endif
ifeq ($(OS),Darwin)
	SUDO:=
	MOUNTPOINT:=img
	MOUNTIMG:=hdiutil attach -mountpoint $(MOUNTPOINT) $(BOOT).img
	UMOUNTIMG:=hdiutil detach $(MOUNTPOINT)
endif
ifeq ($(OS),Linux)
	SUDO:=sudo
	MOUNTPOINT:=img
	MOUNTIMG:=$(SUDO) mount -o loop $(BOOT).img $(MOUNTPOINT)
	UMOUNTIMG:=$(SUDO) umount $(MOUNTPOINT)
endif

$(shell test -d $(MOUNTPOINT) || mkdir $(MOUNTPOINT))

export SRCDIR CC LD CFLAGS ASMFLAGS

.PHONY: all clean $(SUBDIRS) $(CLEAN_SUBDIRS) img mount umount run debug jrun jdebug gdb bochsrc

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

clean: $(CLEAN_SUBDIRS)
	rm -f *.img

$(CLEAN_SUBDIRS):
	$(MAKE) -C $(patsubst _clean_%,%,$@) clean

img: $(SUBDIRS)
	rm -f $(BOOT).img
ifeq ($(BOOT),fd)
	bximage -q -mode=create -$(BOOT)=1.44M $(BOOT).img
	dd if=boot/$(BOOT)/boot.bin of=$(BOOT).img bs=512 count=1 conv=notrunc
else
	bximage -q -mode=create -$(BOOT)=128M $(BOOT).img
endif
	$(MOUNTIMG)
	$(SUDO) cp boot/$(BOOT)/loader.bin $(MOUNTPOINT)/LOADER.BIN
	$(SUDO) cp kernel/kernel.elf $(MOUNTPOINT)/KERNEL.ELF
	$(UMOUNTIMG)

mount: $(SUBDIRS) img
	$(MOUNTIMG)

umount:
	$(UMOUNTIMG)

run: img bochsrc
	bochs -q || true

debug: img bochsrc
	bochsdbg -q

jrun: img bochsrc
	bochs -q || true

jdebug: img bochsrc
	bochsdbg -q

gdb: img bochsrc
	bochsgdb -q || true

bochsrc: bochsrc.txt
	cp -f bochsrc.txt bochsrc
ifeq ($(BOOT),fd)
		echo "floppya: 1_44=$(BOOT).img, status=inserted" >> bochsrc
		echo "boot: floppy" >> bochsrc
else
		echo "ata0-master: type=disk, path=$(BOOT).img, mode=flat" >> bochsrc
		echo "boot: disk" >> bochsrc
endif

