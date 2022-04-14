include ../vars.mk
include ../.config

EXCLUDEDIRS:=
SUBDIRS:=$(shell find . -maxdepth 1 -type d)
SUBDIRS:=$(basename $(patsubst ./%,%,$(SUBDIRS)))
SUBDIRS:=$(filter-out $(EXCLUDEDIRS),$(SUBDIRS))
CLEAN_SUBDIRS:=$(addprefix _clean_,$(SUBDIRS))

.PHONY: clean $(SUBDIRS) $(CLEAN_SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

clean: $(CLEAN_SUBDIRS)

$(CLEAN_SUBDIRS):
	$(MAKE) -C $(patsubst _clean_%,%,$@) clean
