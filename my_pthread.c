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
ucontext_t * runningContext;
ucontext_t * signalContext;

int initialized = 0;
volatile int FLAG =0;
int schedulerCallLock =0;

const size_t TCB_SIZE = sizeof(tcb_t);
const size_t MUTEX_SIZE = sizeof(my_pthread_mutex_t);	
params_t params;
my_scheduler_t * scheduler;
/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
	/*Upon first time calling, initialize scheduler and pass control over to scheduler*/
	if (!initialized) {
		DEBUG_PRINT(("Initializing Scheduler\n"));	
		initialized = 1;
		/*Make of copy of itself, so that scheduler can warp it into a user thread*/
		runningContext = malloc(sizeof(ucontext_t));
		signalContext = malloc(sizeof(ucontext_t));
		
		// Create context for scheduler
		schedulerContext= malloc(sizeof(ucontext_t));
		getcontext(schedulerContext);
		schedulerContext->uc_link = NULL;
		schedulerContext->uc_stack.ss_sp = malloc(10*STACK_SIZE);
		schedulerContext->uc_stack.ss_size = 10*STACK_SIZE;
		schedulerContext->uc_stack.ss_flags = 0;		
		makecontext(schedulerContext, (void*)&my_scheduler_initialize, 0, arg);
		
		//Immediate yields to scheduler.
		swapcontext(runningContext,schedulerContext);		
		DEBUG_PRINT(("Scheduler Initialized. Original Thread resumed. \n"));	
	}
	params.thread = thread;
	params.function_ptr = (void (*)(void))function;
	params.arg = arg;
	FLAG = 2;
	DEBUG_PRINT(("Create thread Called. %p\n", thread));	
	while(1){
		
	}
	swapcontext(runningContext,schedulerContext);
	return 0;
};

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	
	printf("Yield called\n");
	swapcontext(signalContext,schedulerContext);
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

/*Alarm handler*/
void sighandler (int sig){
	DEBUG_PRINT(("ALARM triggered, FLAG: %d. \n", FLAG));
	if(FLAG == 0){
		swapcontext(runningContext, schedulerContext);
	}else if(FLAG ==1){
		DEBUG_PRINT(("ALARM triggered, FLAG: %d. \n", FLAG));
	}
			
}
/*Scheduler functions*/
int my_scheduler_initialize(){
	DEBUG_PRINT(("Initizlied value in scheduler %d\n", initialized));	
	/*record time this is called*/
	clock_t tempClock = clock();
	
	/*Setting constants*/
	scheduler = (my_scheduler_t*)malloc(sizeof(my_scheduler_t));
	/*Scheduler variables*/
	scheduler->runningQueues = (node_t **) malloc(LEVELS_NUM*sizeof(node_t*));
	int i;
	DEBUG_PRINT(("Flag0"));	
	for (i=0;i<LEVELS_NUM;i++){
		scheduler->runningQueues[i] = NULL;
	}
	DEBUG_PRINT(("Flag1"));	
	scheduler-> waitingQueue = NULL;
	scheduler-> all_threads = NULL;
	scheduler-> lastMaintainence = tempClock;
	scheduler-> threadCount =0;
	
	/*Prepare timer and signal handlers*/
	scheduler-> alarmClock = (struct itimerval *) calloc(1, sizeof(struct itimerval));
	scheduler-> alarmClock-> it_interval.tv_sec = 0;
	scheduler-> alarmClock-> it_value.tv_sec=2;
	
	/*Set context to scheduler*/
	scheduler->act.sa_handler = sighandler;
	sigemptyset(&(scheduler->act).sa_mask);
	scheduler->act.sa_flags = 0;
	
	/*Wrap oringal context into a thread*/
	tcb_t * originalThreadTCB = (tcb_t *)malloc(sizeof(tcb_t));
	ucontext_t * newContext = (ucontext_t *) malloc(sizeof(ucontext_t)) ;
	memcpy (newContext, runningContext,sizeof(ucontext_t));
	originalThreadTCB-> context = newContext;
	originalThreadTCB-> TID = 1;		
	originalThreadTCB->createdTime = tempClock;
	originalThreadTCB->totalCPUTime = 0;
	originalThreadTCB->startTime = tempClock;
	originalThreadTCB->stopTime = tempClock;
	originalThreadTCB->priority = 0;
	originalThreadTCB->waiting_lock = NULL;
	originalThreadTCB->holding_locks = NULL;
	
	LL_append(&(scheduler->all_threads), (void*)originalThreadTCB);
	scheduler->threadCount ++;
	LL_append(&(scheduler->runningQueues[0]), (void*)originalThreadTCB);
	
	/*Schedule the original thread*/	
	DEBUG_PRINT(("clock_t %d \n", (double)originalThreadTCB->createdTime));
	sigaction(SIGVTALRM, &scheduler->act, &scheduler->oact);
	setitimer(ITIMER_VIRTUAL, scheduler->alarmClock, NULL);	
	DEBUG_PRINT(("DEBUG version 3. \n"));
	//swapcontext(schedulerContext,((tcb_t*)runningQu	eues[0]->data)->context);
	swapcontext(schedulerContext, newContext);
};

int my_scheduler_newThread(my_pthread_mutex_t *mutex,void *(*function)(void*), void * arg){
	DEBUG_PRINT(("Creating new Thread. \n"));
	ucontext_t * newThreadContext = malloc(sizeof(ucontext_t));
	getcontext(newThreadContext);
	schedulerContext->uc_link = schedulerContext;
	schedulerContext->uc_stack.ss_sp = malloc(STACK_SIZE);
	schedulerContext->uc_stack.ss_size = STACK_SIZE;
	schedulerContext->uc_stack.ss_flags = 0;		
	makecontext(schedulerContext, (void (*)(void))function, 1, arg);
	
	//Immediate yields to scheduler.
	swapcontext(runningContext,schedulerContext);	
}

/* typedef struct Node {
	void * data;
	struct Node * next;
}node_t; */








/*Linked list methods*/
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
	