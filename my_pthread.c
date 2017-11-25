// File:	my_pthread.c
// Author:	Xiaochen Li
// Date:	10/17/2017

// name: Xiaochen Li
// username of iLab: xl234	
// iLab Server: composite.cs.rutgers.edu


//#define DEBUG 1
#ifdef DEBUG
# define DEBUG_PRINT(x) printf x
#else
# define DEBUG_PRINT(x) do {} while (0)
#endif
#include "my_pthread_t.h"
#include "./MyMemory/my_memory.h"

#define STACK_SIZE 64000
#define LEVELS_NUM 20
#define BASE_INTERVAL 25000 //2.5ms
#define MAINTAINENCE_INTERVAL 1000*BASE_INTERVAL

ucontext_t * schedulerContext;
ucontext_t * runningContext;

/*Entry points*/
ucontext_t * SCHEDULE;
ucontext_t * ENDTHREAD;
ucontext_t * EXIT; 	
ucontext_t * NEWTHREAD;
ucontext_t * INITLOCK;
ucontext_t * DESTROYLOCK;
ucontext_t * YIELD;
ucontext_t * JOIN;
ucontext_t * DEQUEUE;
ucontext_t * REQUEUE;

sigset_t * sigvtalrm_set;
my_pthread_t* thread_handle;
my_pthread_mutex_t * mutex_handle;

int DEBUG_endThread_count;

int initialized2 = 0;
volatile int FLAG =0;
int schedulerCallLock =0;

int thread_counter = 0;
/* 
const size_t TCB_SIZE = sizeof(tcb_t);
const size_t MUTEX_SIZE = sizeof(my_pthread_mutex_t);	 */
ucontext_t * newThreadContext;
my_scheduler_t * scheduler;

/*ALl while(1){} are debug clocking codes, which should not appear on the code unlesss I have submitted a wrong version*/

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
	/*Upon first time calling, initialize scheduler and pass control over to scheduler*/
	DEBUG_PRINT(("pthread_create called.\n"));	
	if (!initialized2) {
		DEBUG_PRINT(("Initializing Scheduler.\n"));	
		initialized2 = 1;
		/*Make of copy of itself, so that scheduler can warp it into a user thread*/
		runningContext = myallocate(sizeof(ucontext_t), __FILE__, __LINE__, LIBRARYREQ);
		SCHEDULE = myallocate(sizeof(ucontext_t), __FILE__, __LINE__, LIBRARYREQ);
	
		// Create context for scheduler
		schedulerContext= malloc(sizeof(ucontext_t));
		getcontext(schedulerContext);
		schedulerContext->uc_link = NULL;
		schedulerContext->uc_stack.ss_sp = malloc(10*STACK_SIZE);
		schedulerContext->uc_stack.ss_size = 10*STACK_SIZE;
		schedulerContext->uc_stack.ss_flags = 0;
		
		makecontext(schedulerContext, (void*)&my_scheduler_initialize, 0, NULL);		
		//Immediate yields to scheduler.
		swapcontext(runningContext,schedulerContext);		
		DEBUG_PRINT(("Scheduler Initialized. Original Thread resumed. \n"));	
	}	
	//blocks interruption
	sigprocmask(SIG_BLOCK, sigvtalrm_set, NULL);
	newThreadContext = (ucontext_t*)malloc(sizeof(ucontext_t));
	getcontext(newThreadContext);
	FLAG = 2;
	newThreadContext->uc_link = ENDTHREAD;
	newThreadContext->uc_stack.ss_sp = malloc(STACK_SIZE);
	newThreadContext->uc_stack.ss_size = STACK_SIZE;
	newThreadContext->uc_stack.ss_flags = 0;
	thread_handle = thread;
	makecontext(newThreadContext, (void (*)(void))function, 1, arg);
	swapcontext(runningContext,NEWTHREAD);
	//Returns interruption
	sigprocmask(SIG_UNBLOCK, sigvtalrm_set, NULL);
	//while(1){}
	return 0;
};

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	sigprocmask(SIG_BLOCK, sigvtalrm_set, NULL);
	printf("Yield called\n");
	//swapcontext(runningContext,YIELD);
	sigprocmask(SIG_UNBLOCK, sigvtalrm_set, NULL);
	return 0;
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr) {
	sigprocmask(SIG_BLOCK, sigvtalrm_set, NULL);
	DEBUG_PRINT(("exit called on value %p\n",value_ptr));
	swapcontext(runningContext,ENDTHREAD);
	sigprocmask(SIG_UNBLOCK, sigvtalrm_set, NULL);
};

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {
	sigprocmask(SIG_BLOCK, sigvtalrm_set, NULL);
	DEBUG_PRINT(("Join called on thread %p\n", thread));	
	thread_handle = &thread;
	swapcontext(runningContext,JOIN);
	sigprocmask(SIG_UNBLOCK, sigvtalrm_set, NULL);
	DEBUG_PRINT(("Thread resumed from join: %p\n", thread));	
	return 0;
};

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
	if (!initialized2) {
		DEBUG_PRINT(("Initializing Scheduler, called from mutex_init\n"));	
		initialized2 = 1;
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
		
		makecontext(schedulerContext, (void*)&my_scheduler_initialize, 0, NULL);		
		//Immediate yields to scheduler.
		swapcontext(runningContext,schedulerContext);		
		DEBUG_PRINT(("Scheduler Initialized from mutex_init. Original Thread resumed. \n"));	
	}
	//Critical()
	sigprocmask(SIG_BLOCK, sigvtalrm_set, NULL);
	DEBUG_PRINT(("mutex_init  called on value: %p\n",mutex));
	mutex_handle = mutex;
	swapcontext(runningContext,INITLOCK);
	
	//End of Critical();
	sigprocmask(SIG_UNBLOCK, sigvtalrm_set, NULL);
	DEBUG_PRINT(("mutex_init  return: %p\n",mutex));
	return 0;
};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
	//DEBUG_PRINT(("mutex_lock called. \n"));
	sigprocmask(SIG_BLOCK, sigvtalrm_set, NULL);
	
	//DEBUG_PRINT(("mutex_lock  called on lock %p by Thread: %p\n",*mutex,scheduler->runningThreadTCB->TID));
	//DEBUG_PRINT(("Currente Thread ID:  %p\n",scheduler->runningThreadTCB->TID));
	
	if(((Mutex*)*mutex)->locked ==1){
		DEBUG_PRINT(("Lock already claimed by Thread:  %p\n",((Mutex*)*mutex)->owner));
		LL_append(&(((Mutex*)*mutex)->wait_list),scheduler->runningThreadTCB);
		DEBUG_PRINT(("Waiting list: %p\n",((((Mutex*)*mutex)->wait_list)->data)));
		DEBUG_PRINT(("jumping to DEQUEUE\n"));
		swapcontext(runningContext,DEQUEUE);
		if(((Mutex*)*mutex)->locked ==1){
			DEBUG_PRINT(("Lock still claimed by:  %p. Something wrong\n",((Mutex*)*mutex)->owner));
			while(1){}
			
		}else{
			((Mutex*)*mutex)->locked = 1;
			((Mutex*)*mutex)-> owner = scheduler -> runningThreadTCB;
			LL_push(&(scheduler->runningThreadTCB->holding_locks),mutex);
			
			//DEBUG_PRINT(("Lock Acquired by Thread:  %p\n",((Mutex*)*mutex)->owner));
		}
	}else{
		((Mutex*)*mutex)->locked = 1;
		((Mutex*)*mutex)-> owner = scheduler -> runningThreadTCB;
		LL_push(&(scheduler->runningThreadTCB->holding_locks),mutex);
		//DEBUG_PRINT(("Lock Acquired by Thread:  %p\n",((Mutex*)*mutex)->owner));
	}
	sigprocmask(SIG_UNBLOCK, sigvtalrm_set, NULL);
	return 0;
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
	
	sigprocmask(SIG_BLOCK, sigvtalrm_set, NULL);
	//DEBUG_PRINT(("mutex_unlock  called on lock %p by Thread: %p\n",*mutex,scheduler->runningThreadTCB->TID));
	//DEBUG_PRINT(("mutex_unlock  called on lock %p\n",*mutex));
	//DEBUG_PRINT(("Currente Thread ID:  %p\n",scheduler->runningThreadTCB->TID));
	
	if(((Mutex*)*mutex)->owner !=scheduler->runningThreadTCB->TID){
		DEBUG_PRINT(("Owner doen't match, something's wrong.\n"));
		DEBUG_PRINT(("Currente lock owner:  %p\n",((Mutex*)*mutex)->owner));
		while(1){
			
		}
	}else{
		((Mutex*)*mutex)->locked = 0;
		((Mutex*)*mutex)-> owner = NULL;
		LL_remove(&(scheduler->runningThreadTCB->holding_locks),mutex);
		if((((Mutex*)*mutex)->wait_list)!=NULL){
			DEBUG_PRINT(("Lock Waiting list not NULL %p . \n",((((Mutex*)*mutex)->wait_list)->data)));
			printAllThreadsForLock(mutex);
			/* tcb_t* thread = ((((Mutex*)*mutex)->wait_list)->data);
			LL_remove(&(((Mutex*)*mutex)->wait_list),thread);
			requeueThread(thread); */
			my_pthread_yield();
		}
		//DEBUG_PRINT(("Lock released by Thread:  %p\n",scheduler->runningThreadTCB->TID));
	}
	return 0;
	sigprocmask(SIG_UNBLOCK, sigvtalrm_set, NULL);	
};

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
	sigprocmask(SIG_BLOCK, sigvtalrm_set, NULL);
	/* DEBUG_PRINT(("mutex_destroy  called on value %p\n",mutex));
	
	DEBUG_PRINT(("Info!! \n"));
				int count = 0;
				node_t* current = ((Mutex*)mutex)->wait_list;
				while(current!=NULL){
					DEBUG_PRINT(("Thread::  %p\n",((tcb_t *)(current->data))->TID));
					current = current->next;
					count++;					
				}
				DEBUG_PRINT(("Lock Waiting Thread Count:  %d\n",count));
				 */
				 
	LL_remove(&(scheduler->all_locks),(void*) mutex);
	//printAllThreads();
				
				
	
	//wait_list
	sigprocmask(SIG_UNBLOCK, sigvtalrm_set, NULL);	
	return 0;
};

/*Alarm thread_handler*/
void sighandler (int sig){
	DEBUG_PRINT(("Time to run Scheduler. ALARM triggered, FLAG: %d. \n", FLAG));	
	swapcontext(runningContext, SCHEDULE);			
}
/*Scheduler functions*/
void my_scheduler_initialize(){
	DEBUG_PRINT(("Initizlied value in scheduler:  %d\n", initialized2));	
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
	initKernalContext(NEWTHREAD);
	makecontext(NEWTHREAD, (void*)&my_scheduler_newThread, 0, NULL);
	
	//ENDTHREAD
	ENDTHREAD = (ucontext_t*) malloc(sizeof(ucontext_t));
	initKernalContext(ENDTHREAD);
	makecontext(ENDTHREAD, (void*)&my_scheduler_endThread, 0, NULL);

	//SCHEDULE
	SCHEDULE = (ucontext_t*) malloc(sizeof(ucontext_t));
	initKernalContext(SCHEDULE);
	makecontext(SCHEDULE, (void*)&my_scheduler_schedule, 0, NULL);
	
	
	
	//INITLOCK
	INITLOCK = (ucontext_t*) malloc(sizeof(ucontext_t));
	initKernalContext(INITLOCK);
	makecontext(INITLOCK, (void*)&my_scheduler_initLock, 0, NULL);
	
	//DESTROYLOCK
	DESTROYLOCK = (ucontext_t*) malloc(sizeof(ucontext_t));
	initKernalContext(DESTROYLOCK);
	makecontext(DESTROYLOCK, (void*)&my_scheduler_destoryLock, 0, NULL);
		
	// YIELD
	YIELD = (ucontext_t*) malloc(sizeof(ucontext_t));
	initKernalContext(YIELD);
	makecontext(YIELD, (void*)&my_scheduler_yield, 0, NULL);
	
	// JOIN;
	JOIN = (ucontext_t*) malloc(sizeof(ucontext_t));
	initKernalContext(JOIN);
	makecontext(JOIN, (void*)&my_scheduler_join, 0, NULL);
	
	//DEQUEUE
	DEQUEUE = (ucontext_t*) malloc(sizeof(ucontext_t));
	initKernalContext(DEQUEUE);
	makecontext(DEQUEUE, (void*)&my_scheduler_dequeue, 0, NULL);
	
	//REQUEUE;
	REQUEUE = (ucontext_t*) malloc(sizeof(ucontext_t));
	initKernalContext(REQUEUE);
	makecontext(REQUEUE, (void*)&my_scheduler_requeue, 0, NULL);
	
	DEBUG_PRINT(("Flag3\n"));
	/*Wrap oringal context into a thread*/
	tcb_t * originalThreadTCB = (tcb_t *)malloc(sizeof(tcb_t));
	ucontext_t * newContext = (ucontext_t *) malloc(sizeof(ucontext_t)) ;
	memcpy (newContext, runningContext,sizeof(ucontext_t));
	originalThreadTCB-> context = newContext;
	originalThreadTCB-> TID = (my_pthread_t)originalThreadTCB;		
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
	DEBUG_PRINT(("originalThreadTCB %p \n", originalThreadTCB->TID));
	
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
	
	tcb_t * newThreadTCB = (tcb_t *)malloc(sizeof(tcb_t));
	DEBUG_PRINT(("New Thread Created: %p. \n", newThreadTCB));
	newThreadTCB-> context = newThreadContext;
	newThreadTCB-> TID =  (my_pthread_t)newThreadTCB;		
	newThreadTCB->createdTime = tempClock;
	newThreadTCB->totalCPUTime = 0;
	newThreadTCB->startTime = tempClock;
	newThreadTCB->stopTime = tempClock;
	newThreadTCB->priority = 0;
	newThreadTCB->waiting_lock = NULL;
	newThreadTCB->holding_locks = NULL;
	newThreadTCB -> joinedThreads = NULL;
	newThreadTCB->holding_locks = NULL;
	
	newThreadTCB->tid = thread_counter;
	thread_counter++;
	
	//Set return value
	*thread_handle = newThreadTCB->TID;
	
	//Queueing
	LL_append(&(scheduler->all_threads), (void*)newThreadTCB);
	scheduler->threadCount ++;
	LL_push(&(scheduler->runningQueues[0]), (void*)newThreadTCB);		
	
	//Set timer and go
	scheduleNext();	
}
void my_scheduler_endThread(){
	printAllThreads();
	DEBUG_endThread_count ++;
	DEBUG_PRINT(("End Thread %p. \n", scheduler->runningThreadTCB->TID));
	DEBUG_PRINT(("End Thread Count %d. \n", DEBUG_endThread_count));
	int currentPriority = scheduler->runningThreadTCB->priority;	
	
	//Remove from queues. 
	DEBUG_PRINT(("Thread Priority %d. \n",currentPriority ));
	node_t * temp = scheduler->runningQueues[currentPriority];
	scheduler->runningQueues[currentPriority] =temp->next;
	LL_remove(&(scheduler->all_threads), scheduler->runningThreadTCB);
	
	scheduler->threadCount --;
	
	//Check for locks and joinQueues
	if(scheduler->runningThreadTCB->joinedThreads!=NULL){
		DEBUG_PRINT(("End Thread has someone waiting%p. \n", (scheduler->runningThreadTCB->joinedThreads->data)));
		while(1){}
	}
	if(scheduler->runningThreadTCB->holding_locks!=NULL){
		DEBUG_PRINT(("Running away with locks \n"));
		my_pthread_t* mutex;
		tcb_t* thread;
		//LL_pop(&(scheduler->runningThreadTCB->holding_locks),(void*)&lock);
		DEBUG_PRINT(("1\n"));
		mutex = (my_pthread_t*)scheduler->runningThreadTCB->holding_locks->data;		
		DEBUG_PRINT(("Lock %p\n",mutex));		
		thread = (tcb_t *)((((Mutex*)*mutex)->wait_list)->data);
		LL_remove(&(((Mutex*)*mutex)->wait_list),thread);
		DEBUG_PRINT(("Thread %p\n",thread));
		//DEBUG_PRINT(("Data: %p\n",lock->wait_list->data));	
		
		requeueThread(thread);
		
		DEBUG_PRINT(("3\n"));
	}
	
	
	//Free up memory    
	free(((tcb_t *)(temp->data))->context);
	free(temp->data);
	free(temp);	
	//DEBUG_PRINT(("4\n"));
	//Next thread
	scheduleNext();	
};

//SCHEDULE
void my_scheduler_schedule(){
	//DEBUG_PRINT(("Scheduler: Thread Count %d. \n", scheduler->threadCount));
	//Save current running context
	memcpy (scheduler->runningThreadTCB->context, runningContext,sizeof(ucontext_t));	

	int currentPriority = scheduler->runningThreadTCB->priority;
	
	scheduler->runningThreadTCB->priority ++;
	if( scheduler->runningThreadTCB->priority>=LEVELS_NUM){
		scheduler->runningThreadTCB->priority = 	-1;
	}
	
	node_t * temp = scheduler->runningQueues[currentPriority];
	scheduler->runningQueues[currentPriority] =temp->next; 
	free(temp);
	LL_append(&(scheduler->runningQueues[scheduler->runningThreadTCB->priority]), scheduler->runningThreadTCB);
	printAllThreads();
	scheduleNext();
	
	/* scheduler->runningThreadTCB = (tcb_t*)(scheduler->runningQueues[currentPriority]->data);
	DEBUG_PRINT(("Next scheduled Thread TID %x. \n",scheduler->runningThreadTCB->TID ));
	scheduler-> alarmClock-> it_interval.tv_sec = 0;
	scheduler-> alarmClock-> it_interval.tv_usec = 0;
	scheduler-> alarmClock-> it_value.tv_usec=BASE_INTERVAL+BASE_INTERVAL*currentPriority;
	scheduler-> alarmClock-> it_value.tv_sec=0;
	setitimer(ITIMER_VIRTUAL, scheduler->alarmClock, NULL);	
	
	setcontext(((tcb_t*)(scheduler->runningQueues[currentPriority]->data))->context);  */
};

/* struct Mutex{
	int locked;
	tcb_t * owner;
	node_t * wait_list;
}; */
//INITLOCK
void my_scheduler_initLock(){
	DEBUG_PRINT(("initlock Called:  %p\n",mutex_handle));
	*mutex_handle= (Mutex*)malloc(sizeof(my_pthread_mutex_t));
	((Mutex*)*mutex_handle)->locked = 0;
	((Mutex*)*mutex_handle)->owner = NULL;
	((Mutex*)*mutex_handle)->wait_list = NULL;
	LL_append(&(scheduler->all_locks),(void*)*mutex_handle);
	DEBUG_PRINT(("InitLock Return\n"));
	
	
	setcontext(runningContext);
};

//DEQUEUE put thread to waiting_queue.
void my_scheduler_dequeue(){
	DEBUG_PRINT(("Dequeued called by thread:  %p\n",scheduler->runningThreadTCB->TID));
	
	//Save running context
	memcpy (scheduler->runningThreadTCB->context, runningContext,sizeof(ucontext_t));	
	
	int priority = scheduler->runningThreadTCB->priority;
	
	//Free node memory
	node_t * temp = scheduler->runningQueues[priority];
	scheduler->runningQueues[priority] =temp->next; 
	free(temp);
	
	LL_append(&(scheduler->waitingQueue), scheduler->runningThreadTCB);
	scheduleNext();
}
void my_scheduler_maintainence(){};
void my_scheduler_join(){
	DEBUG_PRINT(("join called by thread:  %p on thread %p.\n",scheduler->runningThreadTCB->TID,*thread_handle));
	memcpy (scheduler->runningThreadTCB->context, runningContext,sizeof(ucontext_t));	
	int priority = scheduler->runningThreadTCB->priority;
	
	if(LL_exists(&(scheduler->all_threads), *thread_handle)){
		DEBUG_PRINT(("Thread exists.\n"));
		//Add to the waiting list of that thread.
		LL_append(&(((tcb_t*)*thread_handle) -> joinedThreads),(void*)scheduler->runningThreadTCB);
		//Free node memory
		node_t * temp = scheduler->runningQueues[priority];
		scheduler->runningQueues[priority] =temp->next; 
		free(temp);	
		//Add to the scheduler list as well
		LL_append(&(scheduler->waitingQueue), scheduler->runningThreadTCB);
		scheduleNext();
	}else{
		DEBUG_PRINT(("Thread doesn't exists. Or may have finished. Resume.\n"));
		setcontext(runningContext);
	}
};
void my_scheduler_destoryLock(){};
void my_scheduler_yield(){
	memcpy (scheduler->runningThreadTCB->context, runningContext,sizeof(ucontext_t));	
	int priority = scheduler->runningThreadTCB->priority;
	
	
	LL_remove(&(scheduler->runningQueues[priority]),scheduler->runningThreadTCB) ;
	LL_append(&(scheduler->runningQueues[priority]), scheduler->runningThreadTCB);
	scheduleNext(); 
};
void my_scheduler_requeue(){};
/* typedef struct Node {
	void * data;
	struct Node * next;	
}node_t; */


void scheduleNext(){
	int priority = 0;
	if(scheduler->runningQueues[priority]==NULL){
		while(scheduler->runningQueues[priority]==NULL){
			priority ++;
			if(priority == LEVELS_NUM){
				//DEBUG_PRINT(("ERROR: No Running thread!! \n"));
				int count = 0;
				node_t* current = scheduler->waitingQueue;
				while(current!=NULL){
					//DEBUG_PRINT(("Thread in Waiting: %p:%p\n",((tcb_t *)(current->data))->TID,((tcb_t *)(current->data))));
					current = current->next;
					count++;					
				}
				//DEBUG_PRINT(("Waiting Thread Count:  %d\n",count));
				while(1){}
			}
		}		
	}
	
	//Change current running thread, set timer and swap
	scheduler->runningThreadTCB = (tcb_t*)(scheduler->runningQueues[priority]->data);
	//DEBUG_PRINT(("Next Scheduled Thread TID %p:%p, priority %d. \n",scheduler->runningThreadTCB->TID,scheduler->runningThreadTCB,scheduler->runningThreadTCB->priority ));
	scheduler-> alarmClock-> it_interval.tv_sec = 0;
	scheduler-> alarmClock-> it_interval.tv_usec = 0;
	scheduler-> alarmClock-> it_value.tv_usec=BASE_INTERVAL+BASE_INTERVAL*priority;
	scheduler-> alarmClock-> it_value.tv_sec=0;
	setitimer(ITIMER_VIRTUAL, scheduler->alarmClock, NULL);	
	//DEBUG_PRINT(("Next Scheduled Thread TID %p, priority %d. \n",scheduler->runningThreadTCB->TID,scheduler->runningThreadTCB->priority ));
	//setcontext(((tcb_t*)(((tcb_t*)(scheduler->runningQueues[priority]->data))->TID))->context);
	
	set_current_thread(((tcb_t*)(scheduler->runningQueues[0]->data))->tid);
	
	setcontext(((((tcb_t*)(scheduler->runningQueues[priority]->data))))->context);
};


//Helper method for initializing kernal contexts
void initKernalContext(ucontext_t* context){
	getcontext(context);
	context->uc_link = NULL;
	context->uc_stack.ss_sp = malloc(STACK_SIZE);
	context->uc_stack.ss_size = STACK_SIZE;
	context->uc_stack.ss_flags = 0;	
	sigaddset(&(context->uc_sigmask), SIGVTALRM);
}

void requeueThread(tcb_t* threadTCB){
	DEBUG_PRINT(("Requeue Thread::  %p with priority: %d \n",threadTCB,threadTCB->priority));
	LL_remove(&(scheduler->waitingQueue),(void*)threadTCB);
	LL_push(&(scheduler->runningQueues[threadTCB->priority]),(void*)threadTCB);
	//while(1){}
}

void printAllThreads(){
	int count = 0;
	node_t * current = scheduler->all_threads;
	while(current!=NULL){
		//DEBUG_PRINT(("Thread::  %p:%p\n",((tcb_t *)(current->data)),((tcb_t *)(current->data))->TID));
		current = current->next;
		count++;					
	}
	//DEBUG_PRINT(("All Thread Count:  %d\n",count));
}


void printAllThreadsForLock(my_pthread_mutex_t* mutex){
	DEBUG_PRINT(("mutex_info of: %p\n",mutex));	
	int count = 0;
	node_t* current;
	tcb_t* thread;
	current = (((Mutex*)*mutex)->wait_list);
	while(current!=NULL){
		thread = (tcb_t*)current->data;
		DEBUG_PRINT(("Thread::  %p, priority: %d\n",thread, thread->priority));
		LL_remove(&(((Mutex*)*mutex)->wait_list),thread);
		requeueThread(thread);
		current = current->next;
		count++;					
	}
	DEBUG_PRINT(("Lock Waiting Thread Count:  %d\n",count));
				 
};


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
int LL_pop(node_t ** listHead, void ** returned_data){
	if(listHead ==NULL){
		returned_data = NULL;
		return 0;
	}else{
		*returned_data = (*listHead)->data;
		node_t * temp = *listHead;
		*listHead = (*listHead)->next;
		free(temp);
		return 1;
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
	
	//DEBUG_PRINT(("Remove called!! target: %p \n",target));
	if(*listHead ==NULL){
		return -1;
	}else{
		node_t * current = *listHead;
		node_t * last = NULL;
		while(current!=NULL){
			//DEBUG_PRINT(("Data: %p \n",current->data));
			if((current->data)!=target){
				last = current;
				current = current->next;			
			}else{
				//DEBUG_PRINT(("Remove Node!! \n"));
				//while(1){}
				if(current == *listHead){
					free(current);	
					*listHead = NULL;
				}else{
					last->next = current->next;
					free(current);
				}
				return 0;
			}
		}
		return -1;
	}
};

//Get a node and remove it from queue.
int LL_exists(	node_t ** listHead, void * target){
	if(*listHead ==NULL){
		return 0;
	}else{
		node_t * current = *listHead;
		node_t * last = NULL;
		while(current!=NULL){
			if(current->data!=target){
				last = current;
				current = current->next;			
			}else{
				return 1;
			}
		}
		return 0;
	}
};
	
