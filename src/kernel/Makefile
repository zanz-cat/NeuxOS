SRC:=$(wildcard *.asm)
OBJ:=$(subst .asm,.o,$(SRC))

SRC1:=$(wildcard *.c)
OBJ1:=$(subst .c,.o,$(SRC1))

.PHOHY: all clean

all: $(OBJ) $(OBJ1)

$(OBJ): $(SRC) $(OBJ1)
	nasm -f elf -o $@ $<

$(OBJ1): $(SRC1) ../include/const.h ../include/string.h ../include/type.h
	gcc -c -m32 -I../include -fno-leading-underscore -o $@ $<

clean:
	rm -f $(OBJ) $(OBJ1)