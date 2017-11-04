#include "my_memory.h"

#define DEBUG_PRINT(x) printf x

int initialized = 0;

// Ptr for start of memory
memblock * memory_head;

int num_pages = 0;
int max_pages = 0;

void * myallocate(size_t size, char * file, int line, int req) {
    DEBUG_PRINT(("myallocate called from %d\n", req));

    if (!initialized) {
        initialize();
    }

    // Size must be > 0
    if (size == 0) {
        DEBUG_PRINT(("Requested size is 0, returning null\n"));
        return NULL;
    }

    // Check if requested size is too large
    if (size > SYSTEM_PAGE_SIZE - sizeof(memblock)) {
        DEBUG_PRINT(("Requested size is greater than system's page size, returning null\n"));
        return NULL;
    }

    // Check if any free pages
    if (num_pages == max_pages) {
        DEBUG_PRINT(("All memory pages are full, returning null\n"));
        return NULL;
    }

    return get_free_memory();
}

void mydeallocate(void* x, char* file, int line, int req) {
    DEBUG_PRINT(("mydeallocate called from %d\n", req));

    memblock * block = (memblock*) x;
    block = block - sizeof(memblock);
    block->free = 1;

    num_pages--;
}


void initialize() {
    max_pages = TOTAL_MEMORY / SYSTEM_PAGE_SIZE;
    DEBUG_PRINT(("Max number of pages is %d\n", max_pages));
	memory_head = (memblock *) sbrk(SYSTEM_PAGE_SIZE);
    DEBUG_PRINT(("Memory head address is %p\n", memory_head));
	memory_head->start = memory_head + sizeof(memblock);
	memory_head->next = NULL;
	memory_head->size = SYSTEM_PAGE_SIZE - sizeof(memblock);
	memory_head->free = 1;
    memory_head->tid = -1;

    create_signal_handler();

	initialized = 1;
}

static void handler(int sig, siginfo_t *si, void *unused) {
    printf("Got SIGSEGV at address: 0x%lx\n",(long) si->si_addr);
    exit(0);
}

void create_signal_handler() {
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = handler;

    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        printf("Fatal error setting up signal handler\n");
        exit(EXIT_FAILURE);    //explode!
    }
}

void * get_free_memory() {
    if (memory_head->free) {
	    DEBUG_PRINT(("Using memory head\n"));
	    memory_head->free = 0;
        return memory_head->start;
    }

    memblock * block = memory_head;
    while (block->next != NULL) {
	    block = block->next;
    	    if (block->free) {
                DEBUG_PRINT(("Using a existing free block\n"));
                block->free = 0;
                return block->start;
	        }
    }

    DEBUG_PRINT(("Creating a new block\n"));
    block->next = (memblock *) sbrk(SYSTEM_PAGE_SIZE);
    block = block->next;
    block->start = block + sizeof(memblock);
    block->next = NULL;
    block->size = SYSTEM_PAGE_SIZE - sizeof(memblock);
    block->free = 0;

    num_pages++;

    return block->start;
}

void protect_memory(void * buffer) {
    DEBUG_PRINT(("protect_memory call on address %p\n", buffer));
    memblock * offset = ((memblock *) buffer) - sizeof(memblock);
    mprotect(offset, SYSTEM_PAGE_SIZE, PROT_NONE);
}

void unprotect_memory(void * buffer) {
    DEBUG_PRINT(("unprotect_memory call on address %p\n", buffer));
    memblock * offset = ((memblock *) buffer) - sizeof(memblock);
    mprotect(offset, SYSTEM_PAGE_SIZE, PROT_READ | PROT_WRITE);
}

void print_num_blocks() {
	int count = 0;
	memblock * block = memory_head;
	while (block != NULL) {
		count++;
		block = block->next;	
	}
	DEBUG_PRINT(("Num blocks %d\n", count));
}
