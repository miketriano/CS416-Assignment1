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

typedef struct Test2 {
	int nums[2000];
} test2;

int main() {

	set_current_thread(1);

    // Malloc 0 bytes - error
    malloc(0);

    // Malloc > system's page size + meta data size - good

    // Malloc test struct and assign values - good
    test * t = malloc(sizeof(test));
    t->num1 = 5;
    t->num2 = 10;
    t->num3 = 15;


    malloc(4 * 1024 * 1024);
    set_current_thread(2);
    malloc(4 * 1024 * 1024);
    set_current_thread(1);
}
