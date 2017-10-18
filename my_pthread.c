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

/*Entry points*/
ucontext_t * SCHEDULE;
ucontext_t * ENDTHREAD;
ucontext_t * EXIT;
ucontext_t * NEWTHREAD;
ucontext_t * NEWLOCK;
ucontext_t * DESTROYLOCK;
ucontext_t * YIELD;
ucontext_t * JOIN;

sigset_t * sigvtalrm_set;
my_pthread_t * handle;


int initialized = 0;
volatile int FLAG =0;
int schedulerCallLock =0;

const size_t TCB_SIZE = sizeof(tcb_t);
const size_t MUTEX_SIZE = sizeof(my_pthread_mutex_t);	
ucontext_t * newThreadContext;
my_scheduler_t * scheduler;
/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
	/*Upon first time calling, initialize scheduler and pass control over to scheduler*/
	if (!initialized) {
		DEBUG_PRINT(("Initializing Scheduler\n"));	
		initialized = 1;
		/*Make of copy of itself, so that scheduler can warp it into a user thread*/
		runningContext = malloc(sizeof(ucontext_t));
		SCHEDULE = malloc(sizeof(ucontext_t));
		
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
	
	
	//blocks interruption
	sigprocmask(SIG_BLOCK, sigvtalrm_set, NULL);
	DEBUG_PRINT(("Create new thread for my_thread_t: %d \n", *thread));	
	newThreadContext = (ucontext_t*)malloc(sizeof(ucontext_t));
	getcontext(newThreadContext);
	FLAG = 2;
	newThreadContext->uc_link = ENDTHREAD;
	newThreadContext->uc_stack.ss_sp = malloc(STACK_SIZE);
	newThreadContext->uc_stack.ss_size = STACK_SIZE;
	newThreadContext->uc_stack.ss_flags = 0;
	handle = thread;
	makecontext(newThreadContext, (void (*)(void))function, 1, (void*) handle);
	swapcontext(runningContext,NEWTHREAD);
	//Returns interruption
	sigprocmask(SIG_UNBLOCK, sigvtalrm_set, NULL);	
	while(1){
		
	}
	swapcontext(runningContext,schedulerContext);
	return 0;
};

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	
	printf("Yield called\n");
	swapcontext(SCHEDULE,schedulerContext);
	return 0;
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr) {
	DEBUG_PRINT(("exit called on value %p\n",value_ptr));	
};

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {
	
	DEBUG_PRINT(("Join called on thread %p\n", &thread));	
	return 0;
};

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
	DEBUG_PRINT(("mutex_init  called on value %p\n",mutex));	
	return 0;
};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
	DEBUG_PRINT(("mutex_lock  called on value %p\n",mutex));
	return 0;
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
	DEBUG_PRINT(("mutex_unlock  called on value %p\n",mutex));
	return 0;
};

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
	DEBUG_PRINT(("mutex_destroy  called on value %p\n",mutex));
	return 0;
};

/*Alarm handler*/
void sighandler (int sig){
	DEBUG_PRINT(("ALARM triggered, FLAG: %d. \n", FLAG));
	if(FLAG == 0){
		//swapcontext(runningContext, schedulerContext);
	}else if(FLAG ==1){
		DEBUG_PRINT(("ALARM triggered, FLAG: %d. \n", FLAG));
	}
			
}
/*Scheduler functions*/
int my_scheduler_initialize(){
	DEBUG_PRINT(("Initizlied value in scheduler:  %d\n", initialized));	
	/*record time this is called*/
	clock_t tempClock = clock();
	/*Setting constants*/
	scheduler = (my_scheduler_t*)malloc(sizeof(my_scheduler_t));
	/*Scheduler variables*/
	

	scheduler->runningQueues = (node_t **) malloc(LEVELS_NUM*sizeof(node_t*));
	int i;
	
	for (i=0;i<LEVELS_NUM;i++){
		scheduler->runningQueues[i] = NULL;
	}
	
	scheduler-> waitingQueue = NULL;
	scheduler-> all_threads = NULL;
	scheduler-> lastMaintainence = tempClock;
	scheduler-> threadCount =0;
	/*Initialize signal mask*/
	sigvtalrm_set = (sigset_t *) calloc(1, sizeof(sigset_t));
	sigaddset(sigvtalrm_set, SIGVTALRM); //Threads with mask_block enabled is not interrupted by Alarms

	/*Prepare timer and signal handlers*/
	scheduler-> alarmClock = (struct itimerval *) calloc(1, sizeof(struct itimerval));
	scheduler-> alarmClock-> it_interval.tv_sec = 0;
	scheduler-> alarmClock-> it_value.tv_sec=2;
	
	
	/*SET alarm signal action*/
	scheduler->act.sa_handler = sighandler;
	sigemptyset(&(scheduler->act).sa_mask);
	scheduler->act.sa_flags = 0;
	
	/*Initialize entry points*/
	
	//NEWTHREAD
	DEBUG_PRINT(("Flag1\n"));
	NEWTHREAD = (ucontext_t*) malloc(sizeof(ucontext_t));
	getcontext(NEWTHREAD);
	NEWTHREAD->uc_link = NULL;
	NEWTHREAD->uc_stack.ss_sp = malloc(STACK_SIZE);
	NEWTHREAD->uc_stack.ss_size = STACK_SIZE;
	NEWTHREAD->uc_stack.ss_flags = 0;	
	sigaddset(&(NEWTHREAD->uc_sigmask), SIGVTALRM);
	DEBUG_PRINT(("Flag2\n"));
	makecontext(NEWTHREAD, (void*)&my_scheduler_newThread, 0, NULL);
	
	//ENDTHREAD
	DEBUG_PRINT(("Flag1\n"));
	ENDTHREAD = (ucontext_t*) malloc(sizeof(ucontext_t));
	getcontext(ENDTHREAD);
	ENDTHREAD->uc_link = NULL;
	ENDTHREAD->uc_stack.ss_sp = malloc(STACK_SIZE);
	ENDTHREAD->uc_stack.ss_size = STACK_SIZE;
	ENDTHREAD->uc_stack.ss_flags = 0;		
	sigaddset(&(ENDTHREAD->uc_sigmask), SIGVTALRM);
	DEBUG_PRINT(("Flag2\n"));
	makecontext(ENDTHREAD, (void*)&my_scheduler_endThread, 1, handle);
	
	
	DEBUG_PRINT(("Flag3\n"));
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
	while(1){
		
	}
};


//NEWTHREAD
int my_scheduler_newThread(my_pthread_t * thread,void *(*function)(void*), void * arg){
	
	DEBUG_PRINT(("Scheduling new thread Thread %d. \n", *((my_pthread_t *)arg)));
	DEBUG_PRINT(("Flag1\n"));
	setcontext(newThreadContext);	
}
int my_scheduler_endThread(my_pthread_t * thread){
	DEBUG_PRINT(("End Thread %d. \n", thread));
};
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
	