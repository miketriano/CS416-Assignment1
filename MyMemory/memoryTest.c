/**
 * Test cases for my_memory
 */

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

    // Malloc, protect, unprotect, write
    test * t2 = malloc(sizeof(test));
    printf("t2 address is %p\n", t2);
    protect_memory(t2);
    unprotect_memory(t2);
    t2->num1 = 99;

    return 0;
}
