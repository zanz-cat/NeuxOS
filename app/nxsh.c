#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <fcntl.h>
#include <dirent.h>

#define BUF_SIZE 1024

void cmd_proc(const char *cmd)
{
    if (strcmp(cmd, "ls") == 0) {
        printf("list dir\n");
        int fd = open("/", 0);
        off_t base;
        char buf[BUF_SIZE];
        if (fd < 0) {
            printf("No such file or directory\n");
            return;
        }
        getdirentries(fd, buf, BUF_SIZE, &base);
    } else if (strcmp(cmd, "exit") == 0) {
        exit(0);
    } else {
        printf("-nxsh %s: command not found\n", cmd);
    }
}

#define PS1 "liwei@NeuxOS$ "
int main(int argc, char *argv[])
{
    char buf[BUF_SIZE];
    int i = 0;
    char ch;

    printf(PS1);
    while (1) {
        ssize_t n = read(0, &ch, 1);
        if (n < 0) {
            printf("read error: %d\n", n);
            return -1;
        }
    
        if (i == 0 && ch == '\b') {
            continue;
        }
        if ((i == BUF_SIZE - 1) && (ch != '\b' && ch != '\n')) {
            continue;
        }
        printf("%c", ch);
        switch (ch) {
            case '\n':
                buf[i] = '\0';
                if (strcmp(buf, "") != 0) {
                    cmd_proc(buf);
                }
                printf(PS1);
                i = 0;
                break;
            case '\b':
                buf[--i] = '\0';
                break;
            default:
                buf[i++] = ch;
                break;
        }
    }
    return 0;
}

void _start(void)
{
    int ret = main(0, NULL);
    exit(ret);
}