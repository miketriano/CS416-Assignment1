// File:	tcb_t.h
// Author:	Xiaochen Li
// Date:	10/17/2017

// name: Xiaochen Li 		
// username of iLab: xl234
// iLab Server: composite.cs.rutgers.edu
#ifndef tcb_t_H
#define tcb_t_H

#define _GNU_SOURCE

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <ucontext.h>
#include <time.h>
#include <string.h>

//Using enum to simplify thread status, RUNNING = 0, etc, etc.
enum thread_status {RUNNING, WAITING, TERMINATED};

typedef uint my_pthread_t;
typedef struct Mutex my_pthread_mutex_t;
typedef struct ThreadControlBlock tcb_t;
//For convenience, tcb_t IS the threadControlBlock, since we are not concerned about memory security. 
//Adding tcb_t's numerical system requires ID distribution and addtional search functionality on linked list.
//Node for Linked List(LL)
typedef struct Node {
	void * data;
	struct Node * next;
}node_t;

struct ThreadControlBlock {
	//tcb_t is a pointer in order to access with different context (hence different stack and variables)
	my_pthread_t TID;
	//Link to context.
	ucontext_t * context;
	
	//Thread stats. All time should be calculated with CPU time, since the process may have been descheduled. clock_t is a structure for CPU time while time_t is a structure for physical time.
	enum thread_status status;
	
	clock_t createdTime;	
	clock_t totalCPUTime; //The amount of time this thread has been on CPU
	clock_t startTime; //The time this thread started running from last cycle. Used for calculating the time this thread 
	clock_t stopTime; //The time this thread is stopped from running, for yield/wait purposes	
	clock_t remainingTime; //If the thread yield(), or is put on wait, its remaining time is calculated.
	
	int priority; //Used for resuming, if it had been put to wait. Also priorirty inversion check if implemented.

	//mutex related
	my_pthread_mutex_t * waiting_lock; //The lock this thread is waiting for to run, if any.
	node_t * holding_locks; //The locks this thread is holding, this is used for priority bump.
	
};



/* mutex struct definition */
struct Mutex{
	int locked;
	tcb_t * owner;
	node_t * wait_list;
};
typedef struct Params{
	my_pthread_t * thread;
	void *(*function)(void*);
	void* arg;
} params_t;

typedef struct Scheduler{
	node_t ** runningQueues;
	node_t * waitingQueue;
	node_t * all_threads;
	int threadCount;
	clock_t lastMaintainence;
	struct itimerval * alarmClock;
	struct sigaction act, oact;
}my_scheduler_t;
/* Function Declarations: */



/*Find() functioins TCB linked lists*/

int tcb_find(tcb_t * thread, void* thread_pointer);

/*My Scheduler Functions*/
void my_scheduler_initialize();
void my_scheduler_newThread(my_pthread_t * thread,void *(*function)(void*), void * arg);
void my_scheduler_endThread(my_pthread_t * thread);
void my_scheduler_maintainence();
void my_scheduler_schedule();
void my_scheduler_newLock();
void my_scheduler_join();
void my_scheduler_exit();
void my_scheduler_destoryLock();
void my_scheduler_yield();

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg);

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield();

/* terminate a thread */
void my_pthread_exit(void *value_ptr);

/* wait for thread termination */
int my_pthread_join(my_pthread_t, void **value_ptr);

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex);

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex);

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex);


/*Linked List(LL) Functions*/
void LL_push(node_t ** listHead, void * new_data);
int LL_pop(node_t ** listHead, void * returned_data);
void LL_append(node_t ** listHead, void * new_data);
int LL_remove(node_t ** listHead, void * target);
#endif
