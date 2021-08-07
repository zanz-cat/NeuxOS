include ../../common.mk

.PHONY: all clean

all: boot.bin loader.bin

boot.bin: boot.S
	nasm $(ASMFLAGS) -o $@ $<

loader.bin: loader.S
	nasm $(ASMFLAGS) -o $@ $<
	@test `stat -c "%s" $@` -gt 65535 && \
	echo "loader too large(`stat -c "%s" $@` > 65535)" && \
	exit 1 || exit 0

clean:
	rm -f boot.bin loader.bin