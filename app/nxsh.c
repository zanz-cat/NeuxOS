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

#define CMDLINE_MAX 64
#define ARGC_MAX 32
#define HISTORY_MAX 32

struct nxsh_cmd {
    const char *name;
    int (*func)(int argc, char *argv[]);
};

static struct nxsh_cmd cmdlist[];

struct nxsh_history {
    char cmdlines[HISTORY_MAX][CMDLINE_MAX];
    int head;
    int tail;
    int cursor;
};
static struct nxsh_history history = {
    .head = HISTORY_MAX - 1,
    .tail = HISTORY_MAX - 1,
    .cursor = 0,
};

struct nxsh_cmdbuf {
    char cmdline[CMDLINE_MAX];
    int cursor;
};

static void cmdbuf_clear(struct nxsh_cmdbuf *buf)
{
    int i;

    for (i = 0; i < buf->cursor; i++) {
        printf("\b");
    }
    buf->cursor = 0;
}

#define HIST_NI(i) (((i) + 1) % HISTORY_MAX)
#define HIST_PI(i) (((i) - 1 + HISTORY_MAX) % HISTORY_MAX)

static void history_append(const char *cmdline)
{
    history.head = HIST_NI(history.head);
    if (history.head == history.tail) {
        history.tail = HIST_NI(history.tail);
    }
    strcpy(history.cmdlines[history.head], cmdline);
    history.cursor = HIST_NI(history.head);
}

static const char *history_prev(void)
{
    if (HIST_PI(history.cursor) == history.tail) {
        return history.cmdlines[history.cursor];
    }
    history.cursor = HIST_PI(history.cursor);
    return history.cmdlines[history.cursor];
}

static const char *history_next(void)
{
    if (history.cursor == HIST_NI(history.head)) {
        return "";
    }
    history.cursor = HIST_NI(history.cursor);
    return history.cmdlines[history.cursor];
}

static void history_clear(void)
{
    history.head = HISTORY_MAX - 1;
    history.tail = HISTORY_MAX - 1;
    history.cursor = 0;
}

static int cmd_history(int argc, char *argv[])
{
    int i, j;

    if (argc == 0) {
        for (i = HIST_NI(history.tail), j = 0; i != HIST_NI(history.head); i = HIST_NI(i), j++) {
            printf("  %d  %s\n", j + 1, history.cmdlines[i]);
        }
        return 0;
    }
    if (strcmp(argv[0], "-c") == 0) {
        history_clear();
    }
    return 0;
}

static int cmd_fork(int argc, char *argv[])
{
    printf("fork return %d\n", fork());
    return 0;
}

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

static int cmd_realpath(int argc, char *argv[])
{
    char resolved[MAX_PATH_LEN];

    if (argc == 0) {
        nxsh_perror("realpath", "missing operand\n");
        return -1;
    }

    if (realpath(argv[0], resolved) == NULL) {
        nxsh_perror("realpath", "%s: %s\n", argv[0], strerror(errno));
        return -1;
    }
    printf("%s\n", resolved);
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

static void nxsh_ctrl_up(struct nxsh_cmdbuf *buf)
{
    cmdbuf_clear(buf);
    strcpy(buf->cmdline, history_prev());
    buf->cursor = strlen(buf->cmdline);
    printf(buf->cmdline);
}

static void nxsh_ctrl_down(struct nxsh_cmdbuf *buf)
{
    cmdbuf_clear(buf);
    strcpy(buf->cmdline, history_next());
    buf->cursor = strlen(buf->cmdline);
    printf(buf->cmdline);
}

static void nxsh_ctrl_left(void)
{

}

static void nxsh_ctrl_right(void)
{

}

static int ctrl(struct nxsh_cmdbuf *buf)
{
    int ret;
    char ch;

    ret = getchar();
    if (ret == EOF) {
        return EOF;
    }
    ch = (char)ret;
    if (ch != '[') {
        return ch;
    }
    ret = getchar();
    if (ret == EOF) {
        return EOF;
    }
    ch = (char)ret;
    switch (ch) {
        case 'A':
            nxsh_ctrl_up(buf);
            break;
        case 'B':
            nxsh_ctrl_down(buf);
            break;
        case 'C':
            nxsh_ctrl_left();
            break;
        case 'D':
            nxsh_ctrl_right();
            break;
        default:
            return ret;
            break;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    int ret;
    char ch;
    struct nxsh_cmdbuf buf = {
        .cursor = 0
    };

    print_PS1();
    while ((ret = getchar()) != EOF) {    
        ch = (char)ret;
        switch (ch) {
            case '\033':
                ret = ctrl(&buf);
                if (ret < 0) {
                    goto eof_error;
                } else if (ret > 0) {
                    // invalid ctrl, print
                    putchar((char)ret);
                }
                break;
            case '\n':
                putchar(ch);
                buf.cmdline[buf.cursor] = '\0';
                if (strlen(buf.cmdline) != 0) {
                    history_append(buf.cmdline);
                    input_proc(buf.cmdline);
                }
                print_PS1();
                buf.cursor = 0;
                break;
            case '\b':
                if (buf.cursor == 0) {
                    break;
                }
                putchar(ch);
                buf.cmdline[--buf.cursor] = '\0';
                break;
            default:
                if (buf.cursor < CMDLINE_MAX - 1) {
                    putchar(ch);
                    buf.cmdline[buf.cursor++] = ch;
                }
                break;
        }
    }

eof_error:
    printf("getchar error: %s\n", strerror(errno));
    return -1;
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
    {"realpath", cmd_realpath},
    {"history", cmd_history},
    {"fork", cmd_fork},
    {NULL, NULL}
};