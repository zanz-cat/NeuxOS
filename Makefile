include common.mk
SUBDIRS:=lib drivers boot fs kernel app
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

.PHONY: all $(SUBDIRS) $(CLEAN_SUBDIRS) image mount umount run \
		debug gdb bochsrc config clean-config install disk clean

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

image: disk install

run: hd.img bochsrc
	bochs -q || true

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
	@rm -f .config
	@echo CONFIG_HD_SZ=$${HD_SZ:-128M} >> .config; \
	echo CONFIG_HD_SECT_SZ=$${HD_SECT_SZ:-512} >> .config; \
	echo CONFIG_HD_HEADS=$${HD_HEADS:-16} >> .config; \
	echo CONFIG_HD_SPT=$${HD_SPT:-63} >> .config; \
	echo CONFIG_EXT2_BS=$${EXT2_BS:-1024} >> .config; \
	echo CONFIG_MEM_PAGE_SZ=$${MEM_PAGE_SZ:-4096} >> .config; \
	echo CONFIG_SHARE_DATA_ADDR=$${SHARE_DATA_ADDR:-0x60000} >> .config;
	@cat .config
	@# non-digital wrapped by ""
	@cat .config | awk -F= '{if($$2 ~ /^[0-9]+$$|^0x[0-9a-fA-F]+$$/) \
		print "#define "$$1" "$$2; else print "#define "$$1" \""$$2"\""}' > config.h
	@cat .config | awk -F= '{if($$2 ~ /^[0-9]+$$|^0x[0-9a-fA-F]+$$/) \
		print $$1" equ "$$2; else print $$1" equ \""$$2"\""}' > config.S

distclean: clean
	rm -f .config config.h config.S