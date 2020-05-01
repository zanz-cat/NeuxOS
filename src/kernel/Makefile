SRC:=$(wildcard *.asm)
OBJ:=$(subst .asm,.o,$(SRC))

SRC1:=$(wildcard *.c)
OBJ1:=$(subst .c,.o,$(SRC1))

.PHOHY: all clean

all: $(OBJ) $(OBJ1)

$(OBJ): $(SRC)
	nasm -f elf -o $@ $(subst .o,.asm,$@)

$(OBJ1): $(SRC1)
	gcc -c -m32 -I../include -fno-leading-underscore -o $@ $(subst .o,.c,$@)

clean:
	rm -f $(OBJ) $(OBJ1)