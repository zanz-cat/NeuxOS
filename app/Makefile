SHELL:=/bin/bash
CC:=gcc
CFLAGS+=-m32 -g -O0 -nostdlib -fno-builtin -fno-leading-underscore
CFLAGS+=-I.. -I../include

SOURCE:=$(wildcard *.c)
BINS:=$(patsubst %.c,%, $(SOURCE))

.PHONY: all clean

all: $(BINS)

$(BINS): %:%.c
	test -d bin || mkdir bin
	$(CC) $(CFLAGS) -o bin/$@ $^ ../lib/posix/libposix.a \
	../lib/c/libc.a ../lib/sys/libsys.a

clean:
	rm -f $(BINS)