#include "stdio.h"
#include "unistd.h"

void app1() {
    static int count = 0;
    while (1) {
        printf("app1: %d\n", count++);
        sleep(10);
    }
}

void app2() {
    static int count = 0;
    while (1) {
        printf(">>>>>>>>>>> app2: %d\n", count++);
        sleep(10);
    }
}