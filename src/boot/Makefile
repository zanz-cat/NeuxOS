.PHOHY: all clean

all: boot.bin loader.bin

boot.bin: boot.asm boot.inc fat12hdr.inc
	nasm -o $@ $<

loader.bin: loader.asm loader.inc boot.inc fat12hdr.inc
	nasm -o $@ $<

clean:
	rm -f boot.bin loader.bin