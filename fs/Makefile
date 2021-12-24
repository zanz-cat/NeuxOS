include ../common.mk

EXCLUDEDIRS:=
SUBDIRS:=$(shell find . -maxdepth 1 -type d)
SUBDIRS:=$(basename $(patsubst ./%,%,$(SUBDIRS)))
SUBDIRS:=$(filter-out $(EXCLUDEDIRS),$(SUBDIRS))
CLEAN_SUBDIRS:=$(addprefix _clean_,$(SUBDIRS))

.PHONY: clean $(SUBDIRS) $(CLEAN_SUBDIRS)

all: $(OBJS) $(SUBDIRS)

clean: $(CLEAN_SUBDIRS)
	rm -f $(OBJS)

$(OBJS): %.o:%.c
	$(CC) -c $(CFLAGS) -o $@ $^

$(SUBDIRS):
	$(MAKE) -C $@

$(CLEAN_SUBDIRS):
	$(MAKE) -C $(patsubst _clean_%,%,$@) clean