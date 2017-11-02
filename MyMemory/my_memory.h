#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

// Memory request called from malloc/free
#define THREADREQ 0

// Memory request called from myallocate/mydeallocate
#define LIBRARYREQ 1

// System page size
#define SYSTEM_PAGE_SIZE sysconf(_SC_PAGE_SIZE)

#define malloc(x) myallocate(x, __FILE__, __LINE__, THREADREQ)
#define free(x) mydeallocate(x, __FILE__, __LINE__, THREADREQ)

typedef struct MetaData {
    void * head;
    void * next;

    // Thread id
    int tid;
} meta_data;

void* myallocate(size_t x, char* file, int line, int req);
void mydeallocate(void* x, char* file, int line, int req);

void initialize();