#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <string.h>
#include <signal.h>

// Memory request called from malloc/free
#define THREADREQ 0

// Memory request called from myallocate/mydeallocate
#define LIBRARYREQ 1

// System page size
#define SYSTEM_PAGE_SIZE sysconf(_SC_PAGE_SIZE)

// 8MB
#define TOTAL_MEMORY 8 * 1024 * 1024

// 16MB
#define SWAP_FILE_MAX_SIZE 16 * 1024 * 1024

#define malloc(x) myallocate(x, __FILE__, __LINE__, THREADREQ)
#define free(x) mydeallocate(x, __FILE__, __LINE__, THREADREQ)
#define shalloc(x) mysharedallocate(x, __FILE__, __LINE__, THREADREQ)

typedef struct MemoryBlock memblock;
typedef struct SwapFile swapfile;

struct MemoryBlock {
	memblock * head;
    void * start;
    memblock * next;

    int size;

    // 1 if free, if allocated
    int free;

    // Thread id
    int tid;
};

struct SwapFile {
	int tid;
	FILE * fp;
	
	swapfile * next;
};

void * myallocate(size_t x, char * file, int line, int req);
void mydeallocate(void * x, char * file, int line, int req);
void * mysharedallocate(size_t x, char * file, int line, int req);

void initialize();
void * get_free_memory();
void print_num_blocks();

void protect_thread();
void unprotect_thread();

void protect_memory(void * buffer);
void unprotect_memory(void * buffer);

void create_signal_handler();

void set_current_thread();

void create_swap_file();
int evict_page();
void write_swap_file();
void read_swap_file();
