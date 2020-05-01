SRC:=$(wildcard *.asm)
OBJ:=$(subst .asm,.o,$(SRC))

.PHOHY: all clean

all: $(OBJ)

$(OBJ): $(SRC)
	nasm -f elf $(subst .o,.asm,$@) -o $@

clean:
	rm -f $(OBJ)