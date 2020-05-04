SRC:=$(wildcard *.asm)
OBJ:=$(subst .asm,.o,$(SRC))

.PHOHY: all clean

all: $(OBJ)

$(OBJ): $(SRC)
	nasm -f elf -o $@ $<

clean:
	rm -f $(OBJ)