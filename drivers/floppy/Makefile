include ../../common.mk

.PHONY: all

all: $(OBJS)

$(OBJS): %.o:%.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean: 
	rm -f $(OBJS)