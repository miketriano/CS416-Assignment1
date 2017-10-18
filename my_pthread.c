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
#define BASE_INTERVAL 25000
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
my_pthread_t * thread_handle;
my_pthread_mutex_t * mutex_handle;


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
	thread_handle = thread;
	makecontext(newThreadContext, (void (*)(void))function, 1, (void*) thread_handle);
	swapcontext(runningContext,NEWTHREAD);
	//Returns interruption
	sigprocmask(SIG_UNBLOCK, sigvtalrm_set, NULL);
	//while(1){}
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
	while(1){}
	return 0;
};

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
	DEBUG_PRINT(("mutex_init  called on value %p\n",mutex));
	DEBUG_PRINT(("Currente Thread ID:  %d\n",scheduler->runningThreadTCB->TID));	
	
	return 0;
};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
	DEBUG_PRINT(("mutex_lock  called on value %p\n",mutex));
	DEBUG_PRINT(("Currente Thread ID:  %d\n",scheduler->runningThreadTCB->TID));	
	return 0;
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
	DEBUG_PRINT(("mutex_unlock  called on value %p\n",mutex));
	DEBUG_PRINT(("Currente Thread ID:  %d\n",scheduler->runningThreadTCB->TID));	
	return 0;
};

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
	DEBUG_PRINT(("mutex_destroy  called on value %p\n",mutex));
	return 0;
};

/*Alarm thread_handler*/
void sighandler (int sig){
	DEBUG_PRINT(("Time to run Scheduler. ALARM triggered, FLAG: %d. \n", FLAG));	
	while(1){
		
	}
	swapcontext(runningContext, SCHEDULE);			
}
/*Scheduler functions*/
void my_scheduler_initialize(){
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

	/*Prepare timer and signal thread_handlers*/
	scheduler-> alarmClock = (struct itimerval *) calloc(1, sizeof(struct itimerval));
	
	
	/*SET alarm signal action*/
	scheduler->act.sa_handler = sighandler;
	sigemptyset(&(scheduler->act).sa_mask);
	scheduler->act.sa_flags = 0;	
	sigaction(SIGVTALRM, &scheduler->act, &scheduler->oact);
	
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
	ENDTHREAD = (ucontext_t*) malloc(sizeof(ucontext_t));
	getcontext(ENDTHREAD);
	ENDTHREAD->uc_link = NULL;
	ENDTHREAD->uc_stack.ss_sp = malloc(STACK_SIZE);
	ENDTHREAD->uc_stack.ss_size = STACK_SIZE;
	ENDTHREAD->uc_stack.ss_flags = 0;		
	sigaddset(&(ENDTHREAD->uc_sigmask), SIGVTALRM);
	makecontext(ENDTHREAD, (void*)&my_scheduler_endThread, 1, thread_handle);

	//SCHEDULE
	SCHEDULE = (ucontext_t*) malloc(sizeof(ucontext_t));
	getcontext(SCHEDULE);
	SCHEDULE->uc_link = NULL;
	SCHEDULE->uc_stack.ss_sp = malloc(STACK_SIZE);
	SCHEDULE->uc_stack.ss_size = STACK_SIZE;
	SCHEDULE->uc_stack.ss_flags = 0;		
	sigaddset(&(SCHEDULE->uc_sigmask), SIGVTALRM);
	makecontext(SCHEDULE, (void*)&my_scheduler_schedule, 1, thread_handle);
	

	//EXIT
	EXIT = (ucontext_t*) malloc(sizeof(ucontext_t));
	getcontext(EXIT);
	EXIT->uc_link = NULL;
	EXIT->uc_stack.ss_sp = malloc(STACK_SIZE);
	EXIT->uc_stack.ss_size = STACK_SIZE;
	EXIT->uc_stack.ss_flags = 0;		
	sigaddset(&(EXIT->uc_sigmask), SIGVTALRM);
	makecontext(EXIT, (void*)&my_scheduler_exit, 1, thread_handle);
	
	
	//NEWLOCK
	NEWLOCK = (ucontext_t*) malloc(sizeof(ucontext_t));
	getcontext(NEWLOCK);
	NEWLOCK->uc_link = NULL;
	NEWLOCK->uc_stack.ss_sp = malloc(STACK_SIZE);
	NEWLOCK->uc_stack.ss_size = STACK_SIZE;
	NEWLOCK->uc_stack.ss_flags = 0;		
	sigaddset(&(NEWLOCK->uc_sigmask), SIGVTALRM);
	makecontext(NEWLOCK, (void*)&my_scheduler_newLock, 1, mutex_handle);
	
	//DESTROYLOCK
	DESTROYLOCK = (ucontext_t*) malloc(sizeof(ucontext_t));
	getcontext(DESTROYLOCK);
	DESTROYLOCK->uc_link = NULL;
	DESTROYLOCK->uc_stack.ss_sp = malloc(STACK_SIZE);
	DESTROYLOCK->uc_stack.ss_size = STACK_SIZE;
	DESTROYLOCK->uc_stack.ss_flags = 0;		
	sigaddset(&(DESTROYLOCK->uc_sigmask), SIGVTALRM);
	makecontext(DESTROYLOCK, (void*)&my_scheduler_destoryLock, 1, mutex_handle);
		
	// YIELD
	YIELD = (ucontext_t*) malloc(sizeof(ucontext_t));
	getcontext(YIELD);
	YIELD->uc_link = NULL;
	YIELD->uc_stack.ss_sp = malloc(STACK_SIZE);
	YIELD->uc_stack.ss_size = STACK_SIZE;
	YIELD->uc_stack.ss_flags = 0;		
	sigaddset(&(YIELD->uc_sigmask), SIGVTALRM);
	makecontext(YIELD, (void*)&my_scheduler_yield, 1, thread_handle);
	
	// JOIN;
	JOIN = (ucontext_t*) malloc(sizeof(ucontext_t));
	getcontext(YIELD);
	JOIN->uc_link = NULL;
	JOIN->uc_stack.ss_sp = malloc(STACK_SIZE);
	JOIN->uc_stack.ss_size = STACK_SIZE;
	JOIN->uc_stack.ss_flags = 0;		
	sigaddset(&(YIELD->uc_sigmask), SIGVTALRM);
	makecontext(YIELD, (void*)&my_scheduler_join, 1, thread_handle);
	
	
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
	originalThreadTCB -> joinedThreads = NULL;
	
	LL_append(&(scheduler->all_threads), (void*)originalThreadTCB);
	scheduler->threadCount ++;
	LL_append(&(scheduler->runningQueues[0]), (void*)originalThreadTCB);
	
	/*Schedule the original thread*/	
	DEBUG_PRINT(("clock_t %d \n", (double)originalThreadTCB->createdTime));
	
	scheduler->runningThreadTCB = originalThreadTCB;
	scheduler-> alarmClock-> it_interval.tv_sec = 0;
	scheduler-> alarmClock-> it_value.tv_sec=2;
	setitimer(ITIMER_VIRTUAL, scheduler->alarmClock, NULL);	
	setcontext(((tcb_t*)(scheduler->runningQueues[0]->data))->context);
	//swapcontext(schedulerContext, newContext);
};

/*Scheduler helper methods*/

//Find next thread in line

//NEWTHREAD
void my_scheduler_newThread(){
	/*Build TCB for new thread*/
	clock_t tempClock = clock();
	
	//Save running context
	memcpy (scheduler->runningThreadTCB->context, runningContext,sizeof(ucontext_t));	
	
	DEBUG_PRINT(("Scheduling new thread Thread %d. \n", *thread_handle));
	tcb_t * newThreadTCB = (tcb_t *)malloc(sizeof(tcb_t));
	newThreadTCB-> context = newThreadContext;
	newThreadTCB-> TID =  *thread_handle;		
	newThreadTCB->createdTime = tempClock;
	newThreadTCB->totalCPUTime = 0;
	newThreadTCB->startTime = tempClock;
	newThreadTCB->stopTime = tempClock;
	newThreadTCB->priority = 0;
	newThreadTCB->waiting_lock = NULL;
	newThreadTCB->holding_locks = NULL;
	newThreadTCB -> joinedThreads = NULL;
	
	//Queueing
	LL_append(&(scheduler->all_threads), (void*)newThreadTCB);
	scheduler->threadCount ++;
	LL_push(&(scheduler->runningQueues[0]), (void*)newThreadTCB);		
	
	//Set timer and go
	scheduler->runningThreadTCB = newThreadTCB;
	scheduler-> alarmClock-> it_interval.tv_sec = 0;
	scheduler-> alarmClock-> it_interval.tv_usec = 0;
	scheduler-> alarmClock-> it_value.tv_usec=BASE_INTERVAL;
	scheduler-> alarmClock-> it_value.tv_sec=0;
	setitimer(ITIMER_VIRTUAL, scheduler->alarmClock, NULL);
	setcontext(((tcb_t*)(scheduler->runningQueues[0]->data))->context);
}
void my_scheduler_endThread(){
	DEBUG_PRINT(("End Thread %d. \n", scheduler->runningThreadTCB->TID));
	int currentPriority = scheduler->runningThreadTCB->priority;	
	
	//Remove from queues.
	DEBUG_PRINT(("Thread Priority %d. \n",currentPriority ));
	node_t * temp = scheduler->runningQueues[currentPriority];
	scheduler->runningQueues[currentPriority] =temp->next;
	LL_remove(&(scheduler->all_threads), scheduler->runningThreadTCB);
	scheduler->threadCount --;
	
	
	//Free up memory
	free(((tcb_t *)(temp->data))->context);
	free(temp->data);
	free(temp);	
	
	
	
	//Next thread
	if(scheduler->runningQueues[currentPriority]==NULL){
		while(scheduler->runningQueues[currentPriority]==NULL){
			currentPriority ++;
			if(currentPriority == LEVELS_NUM) currentPriority = 0;
		}		
	}
	
	scheduler->runningThreadTCB = (tcb_t*)(scheduler->runningQueues[currentPriority]->data);
	DEBUG_PRINT(("New Thread TID %d. \n",currentPriority ));
	scheduler-> alarmClock-> it_interval.tv_sec = 0;
	scheduler-> alarmClock-> it_interval.tv_usec = 0;
	scheduler-> alarmClock-> it_value.tv_usec=BASE_INTERVAL+BASE_INTERVAL*currentPriority;
	scheduler-> alarmClock-> it_value.tv_sec=0;
	setitimer(ITIMER_VIRTUAL, scheduler->alarmClock, NULL);	
	setcontext(((tcb_t*)(scheduler->runningQueues[currentPriority]->data))->context);
	
};

//SCHEDULE
void my_scheduler_schedule(){
	DEBUG_PRINT(("End Thread %d. \n"));
	DEBUG_PRINT(("Thread Count %d. \n", scheduler->threadCount));
	memcpy (scheduler->runningThreadTCB->context, runningContext,sizeof(ucontext_t));	
	while(1){
		
		
	}
	int currentPriority = scheduler->runningThreadTCB->priority;
	scheduler->runningThreadTCB->priority ++;
	if( scheduler->runningThreadTCB->priority==LEVELS_NUM){
		scheduler->runningThreadTCB->priority = LEVELS_NUM-1;
	}
	
	node_t * temp = scheduler->runningQueues[currentPriority];
	scheduler->runningQueues[currentPriority] =temp->next; 
	free(temp);
	
	LL_append(&(scheduler->runningQueues[scheduler->runningThreadTCB->priority]), scheduler->runningThreadTCB);
	
	scheduler->runningThreadTCB = (tcb_t*)(scheduler->runningQueues[currentPriority]->data);
	scheduler-> alarmClock-> it_interval.tv_sec = 0;
	scheduler-> alarmClock-> it_interval.tv_usec = 0;
	scheduler-> alarmClock-> it_value.tv_usec=BASE_INTERVAL+BASE_INTERVAL*currentPriority;
	scheduler-> alarmClock-> it_value.tv_sec=0;
	setitimer(ITIMER_VIRTUAL, scheduler->alarmClock, NULL);	
	setcontext(((tcb_t*)(scheduler->runningQueues[currentPriority]->data))->context);
};

void my_scheduler_maintainence(){};
void my_scheduler_newLock(){};
void my_scheduler_join(){};
void my_scheduler_exit(){};
void my_scheduler_destoryLock(){};
void my_scheduler_yield(){};
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
	