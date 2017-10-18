// File:	my_pthread.c
// Author:	Xiaochen Li
// Date:	10/17/2017

// name: Xiaochen Li
// username of iLab: xl234	
// iLab Server: composite.cs.rutgers.edu
#define DEBUG 1
#ifdef DEBUG
# define DEBUG_PRINT(x) printf x
#else
# define DEBUG_PRINT(x) do {} while (0)
#endif
#include "my_pthread_t.h"

#define STACK_SIZE 64000

ucontext_t * schedulerContext;
ucontext_t * tempContext;
int initialized = 0;

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
	/*Upon first time calling, initialize scheduler and pass control over to scheduler*/
	if (!initialized) {
		DEBUG_PRINT(("Initializing Scheduler\n"));	
		initialized = 1;
		/*Make of copy of itself, so that scheduler can warp it into a user thread*/
		tempContext = malloc(sizeof(ucontext_t));
		getcontext(tempContext);
		
		// Create context for scheduler
		schedulerContext= malloc(sizeof(ucontext_t));
		getcontext(schedulerContext);
		schedulerContext->uc_link = NULL;
		schedulerContext->uc_stack.ss_sp = malloc(STACK_SIZE);
		schedulerContext->uc_stack.ss_size = STACK_SIZE;
		schedulerContext->uc_stack.ss_flags = 0;		
		makecontext(schedulerContext, (void*)&my_scheduler_initialize, 0, arg);
		setcontext(schedulerContext);
	}
	DEBUG_PRINT(("Create thread Called. %p\n", thread));	
	
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
	
	DEBUG_PRINT(("Join called on thread %p\n", &thread));	
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


/*Scheduler functions*/
int my_scheduler_initialize(){
	DEBUG_PRINT(("DEBUG version 2. \n"));
	
	my_scheduler_t * scheduler  = (my_scheduler_t *)malloc (sizeof(my_scheduler_t));
	
	/* typedef struct Scheduler{
		node_t * runningQueues[LEVELS_NUM];
		node_t * waitingQueue;
		node_t * all_threads;
		clock_t lastMaintainence;
		int threadCount;
		
	}my_scheduler_t; */
	int i;
	for (i=0;i<LEVELS_NUM;i++){
		scheduler-> runningQueues[i] = NULL;
	}
	scheduler->waitingQueue = NULL;
	scheduler->all_threads = NULL;
};
