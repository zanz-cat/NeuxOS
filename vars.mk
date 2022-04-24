SHELL:=/bin/bash
CC:=gcc
LD:=ld
ROOTDIR:=$(patsubst %/,%, $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
CFLAGS:=-m32 -g -O0 -nostdlib -fno-builtin -fno-leading-underscore
ifneq ($(strict),no)
	CFLAGS+=-Werror -Wall
endif
CFLAGS+=-I$(ROOTDIR) -I$(ROOTDIR)/include
CFLAGS+=$(EXTRA_CFLAGS)
ASMFLAGS:=-Werror -I$(ROOTDIR)
LDFLAGS:=-m elf_i386

SOURCE:=$(wildcard *.c)
OBJS:=$(patsubst %.c,%.o, $(SOURCE))
ASMSOURCE:=$(wildcard *.S)
ASMOBJS:=$(patsubst %.S,%.o, $(ASMSOURCE))

LIBDIR:=$(ROOTDIR)/lib
DRIVERSDIR:=$(ROOTDIR)/drivers
DEVDIR:=$(ROOTDIR)/dev
FSDIR:=$(ROOTDIR)/fs
MMDIR:=$(ROOTDIR)/mm
ARCHDIR:=$(ROOTDIR)/arch

LIBOBJS:=$(wildcard $(LIBDIR)/*.o) $(LIBDIR)/libc.a
DRIVEROBJS:=$(wildcard $(DRIVERSDIR)/*.o) $(wildcard $(DRIVERSDIR)/*/*.o)
DEVOBJS:=$(wildcard $(DEVDIR)/*.o) $(wildcard $(DEVDIR)/*/*.o)
FSOBJS:=$(wildcard $(FSDIR)/*.o) $(wildcard $(FSDIR)/*/*.o)
MMOBJS:=$(wildcard $(MMDIR)/*.o) $(wildcard $(MMDIR)/*/*.o)
ARCHOBJS:=$(wildcard $(ARCHDIR)/*.o) $(wildcard $(ARCHDIR)/*/*.o)