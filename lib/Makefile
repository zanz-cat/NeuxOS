include ../vars.mk

EXCLUDEDIRS:=
SUBDIRS:=$(shell find . -maxdepth 1 -type d)
SUBDIRS:=$(basename $(patsubst ./%,%,$(SUBDIRS)))
SUBDIRS:=$(filter-out $(EXCLUDEDIRS),$(SUBDIRS))
CLEAN_SUBDIRS:=$(addprefix _clean_,$(SUBDIRS))

.PHONY: all clean $(SUBDIRS) $(CLEAN_SUBDIRS)

CFLAGS += -I$(ROOTDIR)/include

all: $(ASMOBJS) $(OBJS) $(SUBDIRS) libc.a

$(OBJS): %.o:%.c
	$(CC) -c $(CFLAGS) -o $@ $^

$(ASMOBJS): %.o:%.S
	nasm $(ASMFLAGS) -f elf -o $@ $^

clean: $(CLEAN_SUBDIRS)
	rm -f $(OBJS) $(ASMOBJS) libc.a

$(SUBDIRS):
	$(MAKE) -C $@

libc.a: $(ASMOBJS) $(OBJS) $(SUBDIRS)
	ar cr $@ $(ASMOBJS) $(OBJS) */*.o

$(CLEAN_SUBDIRS):
	$(MAKE) -C $(patsubst _clean_%,%,$@) clean