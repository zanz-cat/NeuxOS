SHELL:=/bin/bash
CC:=gcc
LD:=ld
ROOTDIR:=$(patsubst %/,%, $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
CFLAGS:=-m32 -g -Werror -O0 -fno-builtin -fno-leading-underscore -I$(ROOTDIR) -I$(ROOTDIR)/include
ASMFLAGS:=-Werror -I$(ROOTDIR)
LDFLAGS:=-m elf_i386

SOURCE:=$(wildcard *.c)
OBJS:=$(patsubst %.c, %.o, $(SOURCE))
ASMSOURCE:=$(wildcard *.S)
ASMOBJS:=$(patsubst %.S, %_S.o, $(ASMSOURCE))

LIBDIR:=$(ROOTDIR)/lib
DRIVERSDIR:=$(ROOTDIR)/drivers

LIBOBJS:=$(wildcard $(LIBDIR)/*.o)
DRIVEROBJS:=$(wildcard $(DRIVERSDIR)/*.o) $(wildcard $(DRIVERSDIR)/*/*.o)