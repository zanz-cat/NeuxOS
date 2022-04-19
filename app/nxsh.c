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

static void nxsh_perror(const char *cmd, const char *msg, ...)
{
    int ret;
    char buf[255];
    va_list args;

    ret = sprintf(buf, "-nxsh: %s: ", cmd);
    if (ret < 0) {
        return;
    }
    va_start(args, msg);
    (void)vsprintf(buf + ret, msg, args);
    va_end(args);
    printf(buf);
}

static int list_dir(const char *path, int fd)
{
    ssize_t ret;
    off_t base = 0;
    struct stat st;
    struct dirent dent;

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
        break;
    }
    return ret == 0 ? 0 : -1;
}

static int list_file(const char *path, int fd)
{
    printf("%s\n", path);
    return 0;
}

static int list_dent(const char *path)
{
    int ret;
    int fd = -1;
    struct stat st;

    fd = open(path, 0);
    if (fd < 0) {
        goto access_err;
    }
    ret = fstat(fd, &st);
    if (ret != 0) {
        goto access_err;
    }
    ret = (st.st_mode & S_IFDIR) ? list_dir(path, fd) : list_file(path, fd);
    if (ret != 0) {
        printf("ls: cannot list '%s': %s\n", path, strerror(errno));
    }
    goto out;

access_err:
    printf("ls: cannot access '%s': %s\n", path, strerror(errno));
out:
    if (fd >= 0 && close(fd) != 0) {
        printf("ls: cannot close '%s': %s\n", path, strerror(errno));
    }
    return ret == 0 ? 0 : -1;
}

static int cmd_list(int argc, char *argv[])
{
    int i, ret, res;

    if (argc == 0) {
        return list_dent(".");
    }

    ret = 0;
    for (i = 0; i < argc; i++) {
        if (argc > 1) {
            printf("%s:\n", argv[i]);
        }
        res = list_dent(argv[i]);
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
    char buf[MAX_PATH_LEN];
    if (getcwd(buf, MAX_PATH_LEN) == NULL) {
        return -1;
    }
    printf("%s\n", buf);
}

static int cmd_chdir(int argc, char *argv[])
{
    char *path;

    if (argc == 0) {
        return 0;
    }
    path = argv[0];
    if (chdir(path) != 0) {
        goto error;
    }
    return 0;
error:
    nxsh_perror("cd", "%s: %s\n", argv[0], strerror(errno));
    return -1;
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
    nxsh_perror(cmd, "command not found\n");
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
static void print_PS1(void)
{
    char path[MAX_PATH_LEN];

    getcwd(path, MAX_PATH_LEN);
    printf(PS1, strcmp(path, PATH_SEP) == 0 ? PATH_SEP : basename(path));
}

int main(int argc, char *argv[])
{
    char buf[BUF_SIZE];
    int i = 0;
    char ch;

    print_PS1();
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
                print_PS1();
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
    {"cd", cmd_chdir},
    {NULL, NULL}
};