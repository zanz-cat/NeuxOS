SRC:=$(wildcard *.asm)
BIN:=$(subst .asm,.bin,$(SRC))

.PHOHY: all clean

all: $(BIN)

$(BIN): $(SRC)
	nasm $(subst .bin,.asm,$@) -o $@

clean:
	rm -f $(BIN)