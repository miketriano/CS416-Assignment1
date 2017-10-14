// File:	my_pthread.c
// Author:	Yujie REN
// Date:	09/23/2017

// name:
// username of iLab:
// iLab Server:

#include "my_pthread_t.h"

#define MEM 64000
#define THREAD_NUM 10

ucontext_t * mainContext;
int initialized = 0;
int currentThread = 0;
tcb threadBlocks[THREAD_NUM];

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
	
	if (!initialized) {
		initialized = 1;
		mainContext = malloc(sizeof(ucontext_t));
		getcontext(mainContext);
	}
	
	printf("Create thread %p\n", thread);
	
	tcb * threadBlock;
	threadBlock = malloc(sizeof(tcb));
	thread = malloc(sizeof(my_pthread_t));
	threadBlock->pthread = thread;
	
	// create context for thread
	threadBlock->context = malloc(sizeof(ucontext_t));
	getcontext(threadBlock->context);
	threadBlock->context->uc_link = mainContext;
	threadBlock->context->uc_stack.ss_sp = malloc(MEM);
	threadBlock->context->uc_stack.ss_size = MEM;
	threadBlock->context->uc_stack.ss_flags = 0;
	makecontext(threadBlock->context, (void (*)(void))function, 1, arg);
	
	threadBlocks[currentThread] = *threadBlock;
	currentThread++;
	
	return 0;
};

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	
	printf("Yield called\n");
	
	return 0;
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr) {
};

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {
	
	printf("Join called on thread %p\n", &thread);
		
	setcontext(threadBlocks[currentThread].context);
	currentThread++;
	
	return 0;
};

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
	return 0;
};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
	return 0;
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
	return 0;
};

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
	return 0;
};

