#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <lib/misc.h>
#include <config.h>
#include <fs/ext2.h>
#include <kernel/clock.h>
#include <kernel/printk.h>
#include <kernel/sched.h>

int main(int argc, char *argv[])
{
    int count = 0;
    int printed = 0;
    printf("app1: \n");
    while (1) {
        sleep(1);
        for (int i = 0; i < printed; i++) {
            printf("\b");
        }
        printed = printf("%d", count++);
    }
    return 0;
}

void _start(void)
{
    int ret = main(0, NULL);
    exit(ret);
}