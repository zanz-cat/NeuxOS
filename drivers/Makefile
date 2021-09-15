include ../common.mk

EXCLUDEDIRS:=
SUBDIRS:=$(shell find . -maxdepth 1 -type d)
SUBDIRS:=$(basename $(patsubst ./%,%,$(SUBDIRS)))
SUBDIRS:=$(filter-out $(EXCLUDEDIRS),$(SUBDIRS))
CLEAN_SUBDIRS:=$(addprefix _clean_,$(SUBDIRS))

.PHONY: all clean $(SUBDIRS) $(CLEAN_SUBDIRS)

all: $(ASMOBJS) $(SUBDIRS)

$(ASMOBJS): %_S.o:%.S
	nasm $(ASMFLAGS) -f elf -o $@ $^

clean: $(CLEAN_SUBDIRS)
	rm -f $(OBJS) $(ASMOBJS)

$(SUBDIRS):
	$(MAKE) -C $@

$(CLEAN_SUBDIRS):
	$(MAKE) -C $(patsubst _clean_%,%,$@) clean