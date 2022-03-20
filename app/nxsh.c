#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include <fcntl.h>
#include <dirent.h>

#define BUF_SIZE 1024
#define ARGC_MAX 32

struct nxsh_cmd {
    const char *name;
    int (*func)(int argc, char *argv[]);
};

static int list_dir(int argc, char *argv[]);
static int nxsh_exit(int argc, char *argv[]);

static struct nxsh_cmd cmdlist[] = {
    {"ls", list_dir},
    {"exit", nxsh_exit},
    {NULL, NULL}
};

static int list_one_dir(const char *name)
{
    int fd = open(name, 0);
    off_t base = 0;
    ssize_t ret;
    struct dirent dent;
    if (fd < 0) {
        printf("ls: cannot access '%s': No such file or directory\n", name);
        return -ENOENT;
    }
    while (1) {
        ret = getdirentries(fd, (char *)&dent, sizeof(struct dirent), &base);
        if (ret > 0) {
            printf("%s  ", dent.d_name);
            continue;
        }
        if (ret == 0) {
            printf("\n");
            return 0;
        }
        printf("%s\n", strerror(ret));
        return ret;
    }
    ret = close(fd);
    if (ret != 0) {
        printf("close error=%d\n", ret);
        return ret;
    }
    return 0;
}

static int list_dir(int argc, char *argv[])
{
    int i, ret, res;

    if (argc == 0) {
        return list_one_dir("/");
    }

    ret = 0;
    for (i = 0; i < argc; i++) {
        if (argc > 1) {
            printf("%s:\n", argv[i]);
        }
        res = list_one_dir(argv[i]);
        if (res != 0) {
            ret = res;
        }
    }
    return ret;
}

static int nxsh_exit(int argc, char *argv[])
{
    exit(0);
    return 0;
}

int cmd_proc(const char *cmd, int argc, char *argv[])
{
    int i;
    for (i = 0; cmdlist[i].name != NULL; i++) {
        if (strcmp(cmdlist[i].name, cmd) != 0) {
            continue;
        }
        return cmdlist[i].func(argc, argv);
    }
    printf("-nxsh %s: command not found\n", cmd);
    return -1;
}

static int input_proc(char *in)
{
    int i = 0;
    char *token[ARGC_MAX];
    char *s;

    while ((s = strsep(&in, " ")) != NULL) {
        if (strlen(s) == 0) {
            continue;
        }
        token[i++] = s;
    }
    return cmd_proc(token[0], i - 1, &token[1]);
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
                    input_proc(buf);
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