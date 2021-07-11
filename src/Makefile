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

ifeq ($(OS),Darwin)
	SUDO:=
	MOUNTPOINT:=$(SRCDIR)/img
endif
ifeq ($(OS),Linux)
	SUDO:=sudo
	MOUNTPOINT:=$(SRCDIR)/img
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
	# Linux and OSX
	bximage -q -mode=create -$(BOOT)=1.44M $(BOOT).img
	dd if=boot/$(BOOT)/boot.bin of=$(BOOT).img bs=512 count=1 conv=notrunc
	$(SUDO) mount -o loop $(BOOT).img $(MOUNTPOINT)
	$(SUDO) cp boot/$(BOOT)/loader.bin $(MOUNTPOINT)/LOADER.BIN
	$(SUDO) cp kernel/kernel.elf $(MOUNTPOINT)/KERNEL.ELF
	$(SUDO) umount $(MOUNTPOINT)
else
	# harddisk
	bximage -q -mode=create -$(BOOT)=128M $(BOOT).img
ifeq ($(OS),Linux)
	dev=`$(SUDO) losetup -f --show $(BOOT).img`; \
	$(SUDO) fdisk $$dev < hd-fdisk-cmd.txt; \
	offset=`$(SUDO) fdisk -l $$dev | tail -1 | awk '{print $$3}'`; \
	offset=`expr $$offset \* 512`; \
	limit=`$(SUDO) fdisk -l $$dev | tail -1 | awk '{print $$5}'`; \
	limit=`expr $$limit \* 512`; \
	$(SUDO) losetup -d $$dev; \
	dev=`$(SUDO) losetup -f --show -o $$offset --sizelimit $$limit $(BOOT).img`; \
	$(SUDO) mkfs.ext2 $$dev; \
	$(SUDO) mount $$dev $(MOUNTPOINT); \
	$(SUDO) cp boot/$(BOOT)/loader.bin $(MOUNTPOINT)/LOADER.BIN; \
	$(SUDO) cp kernel/kernel.elf $(MOUNTPOINT)/KERNEL.ELF; \
	$(SUDO) umount $(MOUNTPOINT); \
	$(SUDO) losetup -d $$dev;
else
	echo "D"
endif
endif

mount:
ifeq ($(BOOT),fd)
ifeq ($(OS),Linux)
	$(SUDO) mount -o loop $(BOOT).img $(MOUNTPOINT)
else
	hdiutil attach -mountpoint $(MOUNTPOINT) $(BOOT).img
endif
else
ifeq ($(OS),Linux)
	# mount hd on Linux
	offset=`$(SUDO) fdisk -l $(BOOT).img | tail -1 | awk '{print $$3}'`; \
	offset=`expr $$offset \* 512`; \
	limit=`$(SUDO) fdisk -l $(BOOT).img | tail -1 | awk '{print $$5}'`; \
	limit=`expr $$limit \* 512`; \
	dev=`$(SUDO) losetup -f --show -o $$offset --sizelimit $$limit $(BOOT).img`; \
	$(SUDO) mount $$dev $(MOUNTPOINT);
else
	# mount hd on OSX
	echo "D"
endif
endif

umount:
ifeq ($(BOOT),fd)
ifeq ($(OS),Linux)
	$(SUDO) umount $(MOUNTPOINT)
else
	hdiutil detach $(MOUNTPOINT)
endif
else
ifeq ($(OS),Linux)
	$(SUDO) umount $(MOUNTPOINT)
	dev=`$(SUDO) losetup -l -O name -n -j $(BOOT).img`; \
	$(SUDO) losetup -d $$dev;
else
    echo "D"
endif
endif

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

