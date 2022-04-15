include vars.mk
SUBDIRS:=lib arch drivers boot fs mm kernel app
CLEAN_SUBDIRS:=$(addprefix _clean_,$(SUBDIRS))
MOUNTPOINT:=$(ROOTDIR)/rootfs
SUDO:=sudo

ifneq ($(MAKECMDGOALS),config)
ifeq ($(wildcard .config),)
$(error ".config not found, make config first!")
else
include .config
endif
endif

.PHONY: all $(SUBDIRS) $(CLEAN_SUBDIRS) image rootfs mount umount \
		run debug gdb bochsrc config clean-config install disk clean

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

clean: $(CLEAN_SUBDIRS)
	rm -rf bochsrc

$(CLEAN_SUBDIRS):
	$(MAKE) -C $(patsubst _clean_%,%,$@) clean

disk:
	rm -f hd.img
	bximage -q -mode=create -hd=$(CONFIG_HD_SZ) hd.img
	@dev=`$(SUDO) losetup -f --show hd.img`; \
	echo "=> Partition"; \
	echo -e "d\nn\np\n\n\n\na\nw\n" | $(SUDO) fdisk -b $(CONFIG_HD_SECT_SZ) -H $(CONFIG_HD_HEADS) -S $(CONFIG_HD_SPT) $$dev; \
	secsz=`$(SUDO) fdisk -l $$dev | grep 'Sector size' | awk '{print $$4}'`; \
	echo secsz=$$secsz; \
	offset=`$(SUDO) fdisk -l $$dev | tail -1 | awk '{print $$(NF-5)}'`; \
	echo offset=$$offset; \
	offset=`expr $$offset \* $$secsz`; \
	limit=`$(SUDO) fdisk -l $$dev | tail -1 | awk '{print $$(NF-3)}'`; \
	echo limit=$$limit; \
	limit=`expr $$limit \* $$secsz`; \
	$(SUDO) losetup -d $$dev; \
	echo "=> Format"; \
	for i in `seq 10`; \
	do \
		dev=`$(SUDO) losetup -f --show -o $$offset --sizelimit $$limit hd.img` && break || sleep 0.5; \
	done; \
	$(SUDO) mkfs.ext2 -F -b $(CONFIG_EXT2_BS) $$dev;

rootfs:	mount
	@echo "Install rootfs"
	mkdir -p $(MOUNTPOINT)/dev
	@while $(SUDO) mount | grep $(MOUNTPOINT) > /dev/null; \
	do \
	    $(SUDO) umount $(MOUNTPOINT); \
	    dev=`$(SUDO) losetup -l -O name -n -j hd.img` && $(SUDO) losetup -d $$dev; \
	done

install: $(SUBDIRS) hd.img umount
	@test -d $(MOUNTPOINT) || mkdir $(MOUNTPOINT)
	@dev=`$(SUDO) losetup -f --show hd.img`; \
	echo "=> Write MBR"; \
	$(SUDO) dd if=boot/hd/mbr.bin of=$$dev conv=notrunc; \
	secsz=`$(SUDO) fdisk -l $$dev | grep 'Sector size' | awk '{print $$4}'`; \
	echo secsz=$$secsz; \
	offset=`$(SUDO) fdisk -l $$dev | tail -1 | awk '{print $$(NF-5)}'`; \
	echo offset=$$offset; \
	offset=`expr $$offset \* $$secsz`; \
	limit=`$(SUDO) fdisk -l $$dev | tail -1 | awk '{print $$(NF-3)}'`; \
	echo limit=$$limit; \
	limit=`expr $$limit \* $$secsz`; \
	$(SUDO) losetup -d $$dev; \
	for i in `seq 10`; \
	do \
		dev=`$(SUDO) losetup -f --show -o $$offset --sizelimit $$limit hd.img` && break || sleep 0.5; \
	done; \
	echo "=> Write OBR"; \
	$(SUDO) dd if=boot/hd/boot.bin of=$$dev conv=notrunc; \
	$(SUDO) mount $$dev $(MOUNTPOINT); \
	echo "=> Install NeuxOS"; \
	$(SUDO) cp boot/hd/loader.bin $(MOUNTPOINT)/loader.bin; \
	$(SUDO) cp kernel/kernel.elf $(MOUNTPOINT)/kernel.elf; \
	$(SUDO) cp -rf app/bin $(MOUNTPOINT); \
	$(SUDO) ls -lh $(MOUNTPOINT); \
	$(SUDO) umount $(MOUNTPOINT); \
	$(SUDO) losetup -d $$dev;
	@echo "Done!"

mount: hd.img
	@test -d $(MOUNTPOINT) || mkdir $(MOUNTPOINT)
	@$(SUDO) mount | grep $(MOUNTPOINT) > /dev/null && echo "image already mounted" && exit 1 \
	|| echo "mount image to $(MOUNTPOINT)"
	@# mount hd on Linux
	@secsz=`$(SUDO) fdisk -l hd.img | grep 'Sector size' | awk '{print $$4}'`; \
	offset=`$(SUDO) fdisk -l hd.img | tail -1 | awk '{print $$(NF-5)}'`; \
	offset=`expr $$offset \* $$secsz`; \
	limit=`$(SUDO) fdisk -l hd.img | tail -1 | awk '{print $$(NF-3)}'`; \
	limit=`expr $$limit \* $$secsz`; \
	dev=`$(SUDO) losetup -f --show -o $$offset --sizelimit $$limit hd.img`; \
	$(SUDO) mount $$dev $(MOUNTPOINT);

umount:
	@while $(SUDO) mount | grep $(MOUNTPOINT) > /dev/null; \
	do \
	    $(SUDO) umount $(MOUNTPOINT); \
	    dev=`$(SUDO) losetup -l -O name -n -j hd.img` && $(SUDO) losetup -d $$dev; \
	done

image: disk rootfs install

run: hd.img bochsrc
ifeq ($(Q),qemu)
	qemu-system-i386 -m 1024 -smp 1 -drive file=hd.img,format=raw || true
else
	bochs -q || true
endif

debug: hd.img bochsrc
	bochsdbg -q

gdb: hd.img bochsrc
	@echo "gdbstub: enabled=1, port=1234, text_base=0, data_base=0, bss_base=0" >> bochsrc
	bochsgdb -q || true

bochsrc: bochsrc.txt
	cp -f bochsrc.txt bochsrc
	echo "ata0-master: type=disk, path=hd.img, mode=flat" >> bochsrc
	echo "boot: disk" >> bochsrc

config:
	rm -f .config config.h config.S
	@./Kconfig

distclean: clean
	rm -f .config config.h config.S