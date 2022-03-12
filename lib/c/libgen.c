#include <neuxos.h>

#include <libgen.h>

char *dirname(char *path)
{
    int i;
    for (i = 0; path[i] != '\0'; i++);
    for (; i >= 0 && path[i] != PATH_SEP[0]; i--);
    if (i == -1) {
        return ".";
    }
    path[i] = '\0';
    return path;
}

char *basename(char *path)
{
    int i;
    for (i = 0; path[i] != '\0'; i++);
    if (path[i] == PATH_SEP[0]) {
        path[i--] = '\0';
    }
    for (; i >= 0 && path[i] != PATH_SEP[0]; i--);
    if (i == -1) {
        return path;
    }
    return path + i;
}