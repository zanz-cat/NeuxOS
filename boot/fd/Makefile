.PHONY: all clean

all: boot.bin loader.bin

boot.bin: boot.S
	nasm $(ASMFLAGS) -o $@ $<

loader.bin: loader.S
	nasm $(ASMFLAGS) -o $@ $<

clean:
	rm -f boot.bin loader.bin