CURDIR:=$(patsubst %/,%, $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
OUTDIR?=$(CURDIR)
include ../../common.mk
OBJS:=$(patsubst %.c,$(OUTDIR)/%.o, $(SOURCE))

.PHONY: all

all: $(OBJS)

$(OBJS): $(OUTDIR)/%.o:%.c
	$(CC) -c $(CFLAGS) -o $@ $^

clean: 
	rm -f $(OBJS)