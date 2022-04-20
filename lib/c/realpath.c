#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stringex.h>
#include <neuxos.h>
#include <sys/stat.h>

#include <stdlib.h>

char *realpath(const char *path, char *resolved_path)
{
    int i;
    struct stat st;
    char buf[MAX_PATH_LEN] = {0};
    char *token = buf;
    char *dname;

    if (resolved_path == NULL) {
        // resolved_path = malloc(MAX_PATH_LEN);
        errno = -EINVAL;
        return NULL;
    }

    resolved_path[0] = '\0';
    if (!startswith(path, PATH_SEP) && getcwd(resolved_path, MAX_PATH_LEN) == NULL) {
        return NULL;
    }
    if (strcmp(resolved_path, PATH_SEP) == 0) { // root dir
        resolved_path[0] = '\0';
    }

    strcpy(token, path);
    while ((dname = strsep(&token, PATH_SEP)) != NULL) {
        if (strcmp(dname, ".") == 0 || strcmp(dname, "") == 0) {
            continue;
        } else if (strcmp(dname, "..") == 0) {
            for (i = strlen(resolved_path); resolved_path[i] != PATH_SEP[0]; i--);
            resolved_path[i] = '\0';
            continue;
        } else {
            sprintf(resolved_path + strlen(resolved_path), PATH_SEP "%s", dname);
        }
        if (stat(resolved_path, &st) != 0) {
            return NULL;
        }
    }
    if (strlen(resolved_path) == 0) {
        strcpy(resolved_path, PATH_SEP);
    }
    return resolved_path;
}