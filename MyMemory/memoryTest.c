#include <stdio.h>
#include <stdlib.h>

#include "my_memory.h"

typedef struct Test {
	int num1;
	int num2;
	int num3;
} test;

int main() {

    // Malloc 0 bytes - error
    malloc(0);

    // Malloc > system's page size + meta data size - error
    malloc(SYSTEM_PAGE_SIZE);

    // Malloc test struct and assign values - good
    test * t = malloc(sizeof(test));
    t->num1 = 5;
    t->num2 = 10;
    t->num3 = 15;
    printf("Malloc'ed test struct values: num1=%d num2=%d num3=%d\n", t->num1, t->num2, t->num3);

    // Free test struct - good
    free(t);

    // Malloc a bunch of test struct - good
    test * t2 = malloc(sizeof(test));
    t2->num1 = 100;

    test * t3 = malloc(sizeof(test));
    t3->num1 = 100;

    free(t2);

    test * t4 = malloc(sizeof(test));
    t4->num1 = 100;

    return 0;
}
