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
ASMOBJS:=$(patsubst %.S,%_S.o, $(ASMSOURCE))

LIBDIR:=$(ROOTDIR)/lib
DRIVERSDIR:=$(ROOTDIR)/drivers
FSDIR:=$(ROOTDIR)/fs
MMDIR:=$(ROOTDIR)/mm

LIBOBJS:=$(wildcard $(LIBDIR)/*.o) $(LIBDIR)/c/libc.a $(LIBDIR)/posix/libposix.a
DRIVEROBJS:=$(wildcard $(DRIVERSDIR)/*.o) $(wildcard $(DRIVERSDIR)/*/*.o)
FSOBJS:=$(wildcard $(FSDIR)/*.o) $(wildcard $(FSDIR)/*/*.o)
MMOBJS:=$(wildcard $(MMDIR)/*.o) $(wildcard $(MMDIR)/*/*.o)