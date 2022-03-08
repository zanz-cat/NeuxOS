#ifndef __FS_DEV__
#define __FS_DEV__

void devfs_setup(void);
int devfs_mknod(const char *name);

#endif