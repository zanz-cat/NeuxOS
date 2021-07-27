##################################################
# main Makefile
##################################################

OS?=$(shell uname)
CC:=gcc
LD:=ld
SRCDIR:=$(patsubst %/, %, $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
CFLAGS:=-m32 -g -Werror -O0 -I$(SRCDIR)/include -fno-builtin -nostdinc -fno-leading-underscore
ASMFLAGS:=-Werror
SUBDIRS:=app boot lib kernel
CLEAN_SUBDIRS:=$(addprefix _clean_,$(SUBDIRS))
SUDO:=sudo
MOUNTPOINT:=/tmp/neuxos-img

ifneq ($(MAKECMDGOALS),config)
ifeq ($(wildcard .config),)
$(error ".config not found, make config first!")
else
include .config
endif
endif

export SRCDIR CC LD CFLAGS ASMFLAGS

.PHONY: all clean $(SUBDIRS) $(CLEAN_SUBDIRS) img mount umount run debug jrun jdebug gdb bochsrc config

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

clean: $(CLEAN_SUBDIRS)
	rm -rf bochsrc *.img $(MOUNTPOINT)

$(CLEAN_SUBDIRS):
	$(MAKE) -C $(patsubst _clean_%,%,$@) clean

img: $(SUBDIRS)
	test -d $(MOUNTPOINT) || mkdir $(MOUNTPOINT)
	$(SUDO) umount $(MOUNTPOINT) 2>/dev/null || exit 0
	rm -f $(CONFIG_BOOT).img
ifeq ($(CONFIG_BOOT),fd)
	bximage -q -mode=create -$(CONFIG_BOOT)=1.44M $(CONFIG_BOOT).img
	dd if=boot/$(CONFIG_BOOT)/boot.bin of=$(CONFIG_BOOT).img bs=512 count=1 conv=notrunc
	$(SUDO) mount -o loop $(CONFIG_BOOT).img $(MOUNTPOINT)
	$(SUDO) cp boot/$(CONFIG_BOOT)/loader.bin $(MOUNTPOINT)/LOADER.BIN
	$(SUDO) cp kernel/kernel.elf $(MOUNTPOINT)/KERNEL.ELF
	$(SUDO) umount $(MOUNTPOINT)
else
	# harddisk
	bximage -q -mode=create -$(CONFIG_BOOT)=$(CONFIG_HD_SIZE) $(CONFIG_BOOT).img
	dev=`$(SUDO) losetup -f --show $(CONFIG_BOOT).img`; \
	$(SUDO) fdisk $$dev < boot/hd/fdisk-cmd.txt; \
	secsz=`$(SUDO) fdisk -l $$dev | grep 'Sector size' | awk '{print $$4}'`; \
	offset=`$(SUDO) fdisk -l $$dev | tail -1 | awk '{print $$3}'`; \
	offset=`expr $$offset \* $$secsz`; \
	limit=`$(SUDO) fdisk -l $$dev | tail -1 | awk '{print $$5}'`; \
	limit=`expr $$limit \* $$secsz`; \
	$(SUDO) losetup -d $$dev; \
	dev=`$(SUDO) losetup -f --show -o $$offset --sizelimit $$limit $(CONFIG_BOOT).img`; \
	$(SUDO) mkfs.ext2 $$dev; \
	$(SUDO) mount $$dev $(MOUNTPOINT); \
	$(SUDO) cp boot/$(CONFIG_BOOT)/loader.bin $(MOUNTPOINT)/LOADER.BIN; \
	$(SUDO) cp kernel/kernel.elf $(MOUNTPOINT)/KERNEL.ELF; \
	$(SUDO) umount $(MOUNTPOINT); \
	$(SUDO) losetup -d $$dev;
endif

mount:
ifeq ($(CONFIG_BOOT),fd)
	$(SUDO) mount -o loop $(CONFIG_BOOT).img $(MOUNTPOINT)
else
	# mount hd on Linux
	secsz=`$(SUDO) fdisk -l $(CONFIG_BOOT).img | grep 'Sector size' | awk '{print $$4}'`; \
	offset=`$(SUDO) fdisk -l $(CONFIG_BOOT).img | tail -1 | awk '{print $$3}'`; \
	offset=`expr $$offset \* $$secsz`; \
	limit=`$(SUDO) fdisk -l $(CONFIG_BOOT).img | tail -1 | awk '{print $$5}'`; \
	limit=`expr $$limit \* $$secsz`; \
	dev=`$(SUDO) losetup -f --show -o $$offset --sizelimit $$limit $(CONFIG_BOOT).img`; \
	$(SUDO) mount $$dev $(MOUNTPOINT);
endif

umount:
	$(SUDO) umount $(MOUNTPOINT)
ifeq ($(CONFIG_BOOT),hd)
	dev=`$(SUDO) losetup -l -O name -n -j $(CONFIG_BOOT).img`; \
	$(SUDO) losetup -d $$dev;
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
	@echo "gdbstub: enabled=1, port=1234, text_base=0, data_base=0, bss_base=0" >> bochsrc
	bochsgdb -q || true

bochsrc: bochsrc.txt
	cp -f bochsrc.txt bochsrc
ifeq ($(CONFIG_BOOT),fd)
		echo "floppya: 1_44=$(CONFIG_BOOT).img, status=inserted" >> bochsrc
		echo "boot: floppy" >> bochsrc
else
		echo "ata0-master: type=disk, path=$(CONFIG_BOOT).img, mode=flat" >> bochsrc
		echo "boot: disk" >> bochsrc
endif

config:
	@rm -f .config
	@echo CONFIG_BOOT=$${BOOT:-hd} >> .config; \
	if [ "$${BOOT:-hd}" = "hd" ]; then \
		echo CONFIG_HD_SIZE=$${HD_SIZE:-128M} >> .config; \
	fi
	@cat .config

clean-config:
	rm -f .config

show-config:
	@cat .config