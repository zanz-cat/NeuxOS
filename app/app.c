#include <lib/stdio.h>
#include <lib/print.h>
#include <lib/unistd.h>

void app1() 
{
    static int count = 0;
    while (1) {
        sleep(2);
        printf("app1: %d\n", count++);
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
    while (1)
        printk("kapp1: %d\n", count++);
}

void kapp2() 
{
    static int count = 0;
    while (1)
        printf("kapp2: %d\n", count++);
}