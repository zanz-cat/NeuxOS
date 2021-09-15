#include <stdio.h>
#include <unistd.h>

#include <misc/misc.h>
#include <config.h>
#include <fs/ext2.h>
#include <kernel/clock.h>
#include <kernel/printk.h>
#include <kernel/sched.h>

int main(int argc, char *argv[])
{
    static int count = 0;
    int ret;
    while (1) {
        sleep(2);
        ret = printf("app1: %d\n", count++);
        if (ret < 0) {
            break;
        }
    }
    return 0;
}

void _start(void)
{
    main(0, NULL);
}