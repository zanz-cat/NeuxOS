SOURCE:=$(wildcard *.c)
OBJS:=$(patsubst %.c, %.o, $(SOURCE))

.PHONY: all clean

all: $(OBJS)

$(OBJS): %.o:%.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f *.o