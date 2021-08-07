#include <stdio.h>
#include <unistd.h>

#include <kernel/printk.h>

void app1()
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
}

void app2()
{
    static int count = 0;
    while (1) {
        sleep(4);
        printf("app2: %d\n", count++);
    }
}

void kapp1()
{
    static int count = 0;
    while (1) {
        printk("kapp1: %d\n", count++);
    }
}

void kapp2()
{
    static int count = 0;
    while (1) {
        printk("kapp2: %d\n", count++);
    }
}