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
#define LEVELS_NUM 20
#define BASE_INTERVAL
#define  RUN_INTERVAL 1000
#define MAINTAINENCE_INTERVAL 1000*BASE_INTERVAL

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
		
		// Create context for scheduler
		schedulerContext= malloc(sizeof(ucontext_t));
		getcontext(schedulerContext);
		schedulerContext->uc_link = NULL;
		schedulerContext->uc_stack.ss_sp = malloc(10*STACK_SIZE);
		schedulerContext->uc_stack.ss_size = 10*STACK_SIZE;
		schedulerContext->uc_stack.ss_flags = 0;		
		makecontext(schedulerContext, (void*)&my_scheduler_initialize, 0, arg);
		
		//Immediate yields to scheduler.
		swapcontext(tempContext,schedulerContext);		
		DEBUG_PRINT(("Initialized.\n"));	
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
	
	/*record time this is called*/
	clock_t tempClock = clock();
	
	/*Setting constants*/
	const size_t TCB_SIZE = sizeof(tcb_t);
	const size_t MUTEX_SIZE = sizeof(my_pthread_mutex_t);	
	
	/*Scheduler variables*/
	node_t * runningQueues[LEVELS_NUM];
	int i;
	for (i=0;i<LEVELS_NUM;i++){
		runningQueues[i] = NULL;
	}
	node_t * waitingQueue = NULL;
	node_t * all_threads = NULL;
	clock_t lastMaintainence = tempClock;
	int threadCount =0;
	
	/*Prepare timer and signal handlers*/
	struct itimerval * clock = (struct itimerval *) calloc(1, sizeof(struct itimerval));
	clock-> it_interval.tv_sec = 0;
	clock-> it_value.tv_sec=2;
	
	/*Set context to scheduler*/
	void sighandler (int sig){
		DEBUG_PRINT(("ALARM triggered. \n"));
	}
	struct sigaction act, oact;
	act.sa_handler = sighandler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	
	/* struct ThreadControlBlock {
		my_pthread_t ID;
		ucontext_t * context;
		
		clock_t createdTime;	
		clock_t totalCPUTime;
		clock_t startTime;
		clock_t stopTime;
		clock_t remainingTime; 
		
		int priority; 

		my_pthread_mutex_t * waiting_lock; 
		my_pthread_mutex_t ** holding_locks;
		
	}; */
	
	/*Wrap oringal context into a thread*/
	node_t * originalThreadTCB = (node_t *)malloc(sizeof(node_t));
	originalThreadTCB->data = (void*) malloc(sizeof(tcb_t));
	//ucontext_t * newContext = (ucontext_t *) malloc(sizeof(ucontext_t)) ;
	//memcpy (newContext, tempContext,sizeof(ucontext_t));
	((tcb_t *)originalThreadTCB->data)-> context = tempContext;
	//tempContext = NULL;
	((tcb_t *)originalThreadTCB->data)-> TID = 1;		
	((tcb_t *)originalThreadTCB->data)->createdTime = tempClock;
	((tcb_t *)originalThreadTCB->data)->totalCPUTime = 0;
	((tcb_t *)originalThreadTCB->data)->startTime = tempClock;
	((tcb_t *)originalThreadTCB->data)->stopTime = tempClock;
	((tcb_t *)originalThreadTCB->data)->priority = 0;
	((tcb_t *)originalThreadTCB->data)->waiting_lock = NULL;
	((tcb_t *)originalThreadTCB->data)->holding_locks = NULL;
	
	LL_append(&all_threads, (void*)originalThreadTCB);
	threadCount ++;
	LL_append(&runningQueues[0], (void*)originalThreadTCB);
	
	/*Schedule the original thread*/	
	DEBUG_PRINT(("clock_t %d \n", (double)((tcb_t *)originalThreadTCB->data)->createdTime));
	sigaction(SIGVTALRM, &act, &oact);
	setitimer(ITIMER_VIRTUAL, clock, NULL);	
	DEBUG_PRINT(("DEBUG version 2. \n"));
	//DEBUG_PRINT(("Context address %p \n",((tcb_t*)runningQueues[0]->data)->context));	
	swapcontext(schedulerContext,tempContext);	
	
	/*infinite loop to wait for SIGVTALRM*/
};

/* typedef struct Node {
	void * data;
	struct Node * next;
}node_t; */

void LL_push(node_t ** listHead, void * new_data){
	node_t * newNode;
	if(listHead ==NULL){
		newNode = (node_t *) malloc(sizeof(node_t));
		newNode->data = new_data;
		newNode->next = NULL;
	}else{
		newNode = (node_t *) malloc(sizeof(node_t));
		newNode->data = new_data;
		newNode->next = *listHead;
	}
	*listHead = newNode;
};
int LL_pop(node_t ** listHead, void * returned_data){
	if(listHead ==NULL){
		returned_data = NULL;
		return -1;
	}else{
		returned_data = (*listHead)->data;
		node_t * temp = *listHead;
		*listHead = (*listHead)->next;
		free(temp);
		return 0;
	}
};
void LL_append(node_t ** listHead, void * new_data){
	node_t * newNode;
	if(*listHead ==NULL){
		newNode = (node_t *) malloc(sizeof(node_t));
		newNode->data = new_data;
		newNode->next = NULL;
		*listHead = newNode;
	}else{
		node_t * current = *listHead;
		while(current->next!=NULL){
			current = current->next;
		}
		newNode = (node_t *) malloc(sizeof(node_t));
		newNode->data = new_data;
		newNode->next = NULL;
		current->next = newNode;
	}
};

/*The method is not reponsible for freeing void* target*/
int LL_remove(node_t ** listHead, void * target){
	if(*listHead ==NULL){
		return -1;
	}else{
		node_t * current = *listHead;
		node_t * last = NULL;
		while(current->data!=target){
			last = current;
			current = current->next;			
		}
		last->next = current->next;
		
		if(current == *listHead){
			free(current);
			current = NULL;
		}else{
			free(current);
		}
		
		return 0;
	}
};
	