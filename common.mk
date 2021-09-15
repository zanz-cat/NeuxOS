SHELL:=/bin/bash
CC:=gcc
LD:=ld
ROOTDIR:=$(patsubst %/,%, $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
CFLAGS:=-m32 -g -O0 -Werror -Wall -nostdlib -fno-builtin -fno-leading-underscore
CFLAGS+=-I$(ROOTDIR) -I$(ROOTDIR)/include
ASMFLAGS:=-Werror -I$(ROOTDIR)
LDFLAGS:=-m elf_i386

SOURCE:=$(wildcard *.c)
OBJS:=$(patsubst %.c,%.o, $(SOURCE))
ASMSOURCE:=$(wildcard *.S)
ASMOBJS:=$(patsubst %.S,%_S.o, $(ASMSOURCE))

LIBDIR:=$(ROOTDIR)/lib
DRIVERSDIR:=$(ROOTDIR)/drivers
FSDIR:=$(ROOTDIR)/fs

LIBOBJS:=$(wildcard $(LIBDIR)/*.o) $(LIBDIR)/c/libc.a $(LIBDIR)/posix/libposix.a
DRIVEROBJS:=$(wildcard $(DRIVERSDIR)/*.o) $(wildcard $(DRIVERSDIR)/*/*.o)
FSOBJS:=$(wildcard $(FSDIR)/*.o) $(wildcard $(FSDIR)/*/*.o)