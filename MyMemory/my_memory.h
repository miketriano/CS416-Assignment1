#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

// Memory request called from malloc/free
#define THREADREQ 0

// Memory request called from myallocate/mydeallocate
#define LIBRARYREQ 1

// System page size
#define SYSTEM_PAGE_SIZE sysconf(_SC_PAGE_SIZE)

// 8MB
#define TOTAL_MEMORY 8 * 1024 * 1024

#define malloc(x) myallocate(x, __FILE__, __LINE__, THREADREQ)
#define free(x) mydeallocate(x, __FILE__, __LINE__, THREADREQ)

typedef struct MemoryBlock {
    void * start;
    void * next;

    int size;

    // 1 if free, if allocated
    int free;

    // Thread id
    int tid;
} memblock;

void* myallocate(size_t x, char* file, int line, int req);
void mydeallocate(void* x, char* file, int line, int req);

void initialize();
void * get_free_memory();
void print_num_blocks();
