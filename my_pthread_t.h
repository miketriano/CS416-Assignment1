// File:	my_pthread_t.h
// Author:	Xiaochen Li
// Date:	10/17/2017

// name: Xiaochen Li 		
// username of iLab: xl234
// iLab Server: composite.cs.rutgers.edu
#ifndef MY_PTHREAD_T_H
#define MY_PTHREAD_T_H

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

#define LEVELS_NUM 20
#define  RUN_INTERVAL 1000
#define MAINTAINENCE_INTERVAL
//Using enum to simplify thread status, RUNNING = 0, etc, etc.
enum thread_status {RUNNING, WAITING, TERMINATED};

typedef uint my_pthread_t;

typedef struct ThreadControlBlock {
	//My_pthread_t is a pointer in order to access with different context (hence different stack and variables)
	my_pthread_t* self;
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
	my_pthread_mutex_t ** holding_locks; //The locks this thread is holding, this is used for priority bump.
	
}tcb_t; 

//Node for Linked List(LL)
typedef struct Node{
	void * data;
	struct Node * next;
}node_t;

/* mutex struct definition */
typedef struct Mutex{
	int locked = 0;
	my_pthread_t * owner;
	my_pthread_t ** wait_list;
} my_pthread_mutex_t;

typedef struct Scheduler{
	//The context of the scheduler itself.
	ucontext_t * context;
	
	node_t * runningQueues[LEVELS_NUM];
	node_t * waitingQueue;
	node_t * all_threads;
	
	/*Self stats*/
	clock_t lastMaintainence;
	
}my_scheduler_t;

/* Function Declarations: */

/*Linked List(LL) Functions*/
void LL_push(node_t ** listHead, void * new_data, size_t data_size);
int LL_pop(node_t ** listHead, void * get_data, size_t data_size);
void LL_append(node_t ** listHead, void * new_data, size_t data_size);
int LL_get(node_t ** listHead, void * get_data, size_t data_size);
int LL_remove(node_t ** listHead, void * target, size_t data_size);

/*Find() functioins TCB linked lists*/

int tcb_find();

/*My Scheduler Functions*/
int my_scheduler_initialize();

int my_scheduler_maintainence();


/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg);

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield();

/* terminate a thread */
void my_pthread_exit(void *value_ptr);

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr);

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex);

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex);

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex);

#endif
