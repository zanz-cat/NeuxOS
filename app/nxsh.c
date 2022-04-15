#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <libgen.h>

#include <neuxos.h>
#include <stringex.h>

#define BUF_SIZE 1024
#define ARGC_MAX 32

struct nxsh_cmd {
    const char *name;
    int (*func)(int argc, char *argv[]);
};

static struct nxsh_cmd cmdlist[];
static struct {
    char path[NAME_MAX];
    char base[NAME_MAX];
} cwd = {
    .path = "",
    .base = "",
};

static int list_one(const char *name)
{
    int fd;
    off_t base = 0;
    ssize_t ret;
    struct stat st;
    struct dirent dent;
    char buf[NAME_MAX];
    const char *path = name;

    if (!startswith(name, PATH_SEP)) {
        sprintf(buf, "%s" PATH_SEP "%s", cwd.path, name);
        path = buf;
    }
    ret = stat(path, &st);
    if (ret != 0) {
        goto error;
    }
    fd = open(path, 0);
    if (fd < 0) {
        goto error;
    }

    while (1) {
        ret = getdirentries(fd, (char *)&dent, sizeof(struct dirent), &base);
        if (ret > 0) {
            printf("%s  ", dent.d_name);
            continue;
        }
        if (ret == 0) {
            printf("\n");
            break;
        }
        printf("ls: cannot list '%s': %s\n", path, strerror(errno));
        break;
    }
    if (close(fd) != 0) {
        printf("ls: cannot close '%s': %s\n", path, strerror(errno));
        return -1;
    }
    return ret == 0 ? 0 : -1;

error:
    printf("ls: cannot access '%s': %s\n", path, strerror(errno));
    return -1;
}

static int cmd_list(int argc, char *argv[])
{
    int i, ret, res;

    if (argc == 0) {
        return list_one(cwd.path);
    }

    ret = 0;
    for (i = 0; i < argc; i++) {
        if (argc > 1) {
            printf("%s:\n", argv[i]);
        }
        res = list_one(argv[i]);
        if (res != 0) {
            ret = res;
        }
    }
    return ret;
}

static int cmd_exit(int argc, char *argv[])
{
    exit(0);
    return 0;
}

static int cmd_pwd(int argc, char *argv[])
{
    printf("%s\n", strlen(cwd.path) == 0 ? PATH_SEP : cwd.path);
}

static bool rootdir(const char *s)
{
    return strlen(s) == 1 && s[0] == PATH_SEP[0];
}

static int cmd_chg_workdir(int argc, char *argv[])
{
    char *path;
    char buf[NAME_MAX];

    if (argc == 0) {
        return 0;
    }

    path = argv[0];
    if (!startswith(argv[0], PATH_SEP)) {
        sprintf(buf, "%s" PATH_SEP "%s", cwd.path, argv[0]);
        path = buf;
    }
    // stat(argv[0], st)
    if (rootdir(path)) {
        strcpy(cwd.path, "");
        strcpy(cwd.base, "");
    } else {
        strcpy(cwd.path, path);
        strcpy(cwd.base, basename(path));
    }
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

#define PS1 "[liwei@NeuxOS %s]$ "
int main(int argc, char *argv[])
{
    char buf[BUF_SIZE];
    int i = 0;
    char ch;

    printf(PS1, cwd.base);
    while (1) {
        ssize_t n = read(0, &ch, 1);
        if (n < 0) {
            printf("read error: %s\n", strerror(errno));
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
                if (strlen(buf) != 0) {
                    input_proc(buf);
                }
                printf(PS1, cwd.base);
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

static struct nxsh_cmd cmdlist[] = {
    {"ls", cmd_list},
    {"exit", cmd_exit},
    {"pwd", cmd_pwd},
    {"cd", cmd_chg_workdir},
    {NULL, NULL}
};