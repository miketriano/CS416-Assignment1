#include "my_memory.h"

#define DEBUG_PRINT(x) printf x

int initialized = 0;

// Ptr for start of memory
memblock * memory_head;

swapfile * swap_file_head;

// Number of allocated pages
int num_pages = 0;

// Max number of allocated pages
int max_pages = 0;

int current_thread = -1;

/**
 * malloc()
 */
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
		if (!evict_page()) {
			DEBUG_PRINT(("All memory pages are full, returning null\n"));
			return NULL;
		}
    }

    return get_free_memory();
}

/**
 * free()
 */
void mydeallocate(void * x, char * file, int line, int req) {
    DEBUG_PRINT(("mydeallocate called from %d\n", req));

    memblock * block = (memblock*) x;
    block = block - sizeof(memblock);
    block->free = 1;

    num_pages--;
}

void * mysharedallocate(size_t x, char * file, int line, int req) {
	DEBUG_PRINT(("mysharedallocate called\n"));
}

/**
 * Initialize the first memory page
 */
void initialize() {
	DEBUG_PRINT(("initialize called\n"));
    max_pages = TOTAL_MEMORY / SYSTEM_PAGE_SIZE;
    DEBUG_PRINT(("Max number of pages is %d\n", max_pages));
	memory_head = (memblock *) sbrk(SYSTEM_PAGE_SIZE);
    DEBUG_PRINT(("Memory head address is %p\n", memory_head));
    memory_head->head = memory_head;
	memory_head->start = memory_head + sizeof(memblock);
	memory_head->next = NULL;
	memory_head->size = SYSTEM_PAGE_SIZE - sizeof(memblock);
	memory_head->free = 1;
	memory_head->tid = -1;

    create_signal_handler();
    
    create_swap_file();

	initialized = 1;
}

/**
 * Signal handler
 */
static void handler(int sig, siginfo_t *si, void *unused) {
	memblock * offset = ((memblock *) si->si_addr) - sizeof(memblock);
    printf("Got SIGSEGV at address: 0x%lx from thread %d\n",(long) si->si_addr, offset->tid);
    
    // Check if thread was accessing another thread's memory
    if (offset->tid != current_thread) {
		DEBUG_PRINT(("Thread %d tried to access thread's %d memory! Exiting.\n", offset->tid, current_thread));
		exit(0);
	}
	
	// Something else went wrong
	exit(0);
}

/**
 * Create the signal handler
 */
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

/**
 * Find a free memory page and return it
 */
void * get_free_memory() {
    if (memory_head->free) {
	    DEBUG_PRINT(("Using memory head\n"));
	    memory_head->free = 0;
	    memory_head->tid = current_thread;
        return memory_head->start;
    }

    memblock * block = memory_head;
    while (block->next != NULL) {
	    block = block->next;
    	    if (block->free) {
                DEBUG_PRINT(("Using a existing free block\n"));
                block->free = 0;
                block->tid = current_thread;
                return block->start;
	        }
    }

    DEBUG_PRINT(("Creating a new block\n"));
    block->next = (memblock *) sbrk(SYSTEM_PAGE_SIZE);
    block = block->next;
    block->head = block;
    block->start = block + sizeof(memblock);
    block->next = NULL;
    block->size = SYSTEM_PAGE_SIZE - sizeof(memblock);
    block->free = 0;
    block->tid = current_thread;

    num_pages++;

    return block->start;
}

/**
 * Set the current thread
 */
void set_current_thread(int tid) {
	if (!initialized) {
        initialize();
    }
    
    // Protect old thread, unprotect new thread
    if (current_thread != -1) {
		//protect_thread(current_thread);
		//unprotect_thread(tid);
	}
	
	current_thread = tid;
}

/**
 * protect all the thread's memory pages
 */
void protect_thread(int tid) {
	DEBUG_PRINT(("protect_thread called on thread %d\n", tid));
	memblock * block = memory_head;
	while (block != NULL) {
		if (block->tid == tid) {
			mprotect(block, SYSTEM_PAGE_SIZE, PROT_READ);
		}
		block = block->next;
	}
}

/**
 * unprotect all the thread's memory pages
 */
void unprotect_thread(int tid) {
	DEBUG_PRINT(("unprotect_thread called on thread %d\n", tid));
	memblock * block = memory_head;
	while (block != NULL) {
		if (block->tid == tid) {
			mprotect(block, SYSTEM_PAGE_SIZE, PROT_READ | PROT_WRITE);
		}
		block = block->next;
	}
}

/**
 * mprotect a memory space starting at buffer to only allow reads
 */
void protect_memory(void * buffer) {
    DEBUG_PRINT(("protect_memory call on address %p\n", buffer));
    memblock * offset = ((memblock *) buffer) - sizeof(memblock);
    //  Should be PROT_NONE
    mprotect(offset, SYSTEM_PAGE_SIZE, PROT_READ);
}

/**
 * mprotect a memory space starting at buffer to allow reads and writes
 */
void unprotect_memory(void * buffer) {
    DEBUG_PRINT(("unprotect_memory call on address %p\n", buffer));
    memblock * offset = ((memblock *) buffer) - sizeof(memblock);
    mprotect(offset, SYSTEM_PAGE_SIZE, PROT_READ | PROT_WRITE);
}

/**
 * Create a 16MB swap file
 */
void create_swap_file() {
	DEBUG_PRINT(("create_swap_file called\n"));
	int size = 16 * 1024 * 1024 - 1; //16MB
	FILE *fp = fopen("swapfile", "w");
	fseek(fp, size, SEEK_SET);
	fputc('\0', fp);
	fclose(fp);
}

/**
 * Finds page to write to swap file and free
 */
int evict_page() {
	memblock * block = memory_head;
	while (block != NULL) {
		if (block->tid != current_thread) {
			FILE * fp = fopen("swapfile", "r");
			size_t written = 0;
			while (written < SYSTEM_PAGE_SIZE) {
				written = written + fwrite(block, 1, SYSTEM_PAGE_SIZE, fp);
			}
			block->free = 1;
			num_pages--;
			return 1;
		}
	}
	return 0;
}

void write_swap_file() {
	DEBUG_PRINT(("write_swap_file called\n"));
	FILE * fp = fopen("swapfile", "w");
	memblock * block = memory_head;
	while (block != NULL) {
		size_t written = 0;
		while (written < SYSTEM_PAGE_SIZE) {
			written = written + fwrite(block, 1, SYSTEM_PAGE_SIZE, fp);
			DEBUG_PRINT(("Wrote %zu bytes to swap file\n", written));
			
		}
		block = block->next;
	}
	fclose(fp);
}

void read_swap_file() {
	DEBUG_PRINT(("read_swap_file called\n"));
	FILE * fp = fopen("swapfile", "r");
	void * ptr = sbrk(SYSTEM_PAGE_SIZE);
	size_t read = fread(ptr, 1, SYSTEM_PAGE_SIZE, fp);
	DEBUG_PRINT(("Read %zu bytes from swap file\n", read));
	if (read == SYSTEM_PAGE_SIZE) {
		memblock * block = (memblock *) ptr;
		DEBUG_PRINT(("memory head addres %p\n", block->head));
		memcpy(memory_head, ptr, SYSTEM_PAGE_SIZE);
	}	
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
