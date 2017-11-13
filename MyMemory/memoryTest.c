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
    printf("Malloc'ed test struct values: num1=%d num2=%d num3=%d\n", t->num1, t->num2, t->num3);
	free(t);
    // Free test struct - good
    //free(t);

    // Malloc, change threads, write to old thread - bad
    //test * t2 = malloc(sizeof(test));
	//set_current_thread(2);
	//t2->num1 = 99;

	// Shalloc test
	//shalloc(100);
	
	test * t2 = malloc(sizeof(test));
	t2->num1 = 5;
	write_swap_file();
	t2->num1 = 10;
	read_swap_file();
	printf("t2 num1=%d\n", t2->num1);


    return 0;
}
