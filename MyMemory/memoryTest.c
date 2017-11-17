/**
 * Test cases for my_memory
 */

#include <stdio.h>
#include <stdlib.h>

#include "my_memory.h"

#define MB 1 * 1024 * 1024

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

	test * ptr = malloc(4 * MB);
	ptr->num1 = 10;
	set_current_thread(2);
	test * ptr2 = malloc(4 * MB);
	ptr2->num1 = 5;
	set_current_thread(1);
	printf("num1=%d\n", ptr->num1);
	set_current_thread(2);
	printf("num1=%d\n", ptr->num1);
	
	
	test * ptr3 = shalloc(100);
	ptr3->num1 = 100;

	test * ptr4 = shalloc(100);
	ptr4->num1 = 110;
	test * ptr5 = shalloc(100);
	ptr5->num1 = 120;
	shalloc(10);
	
}
