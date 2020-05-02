.PHOHY: all clean

all: boot.bin loader.bin

boot.bin: boot.asm boot.inc fat12hdr.inc
	nasm $(subst .bin,.asm,$@) -o $@

loader.bin: loader.asm loader.inc boot.inc fat12hdr.inc
	nasm $(subst .bin,.asm,$@) -o $@

clean:
	rm -f boot.bin loader.bin