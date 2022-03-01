#include <fs/fs.h>

#include "dev.h"

void dev_setup(void)
{
    vfs_mount("/dev", NULL);
}