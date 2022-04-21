#include <stdio.h>
#include <unistd.h>

int getchar(void)
{
    char ch;
    ssize_t ret;

    ret = read(STDIN_FILENO, &ch, 1);
    if (ret < 0) {
        return EOF;
    }
    return (int)ch;
}

int putchar(int c)
{
    return write(0, &c, 1);
}