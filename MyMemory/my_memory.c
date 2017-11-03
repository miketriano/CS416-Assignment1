#include <malloc.h>
#include <unistd.h>

#include "my_memory.h"


#define DEBUG_PRINT(x) printf x

// Ptr for start of memory
memblock * memory_head;

int initialized = 0;

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

    return get_free_memory();
}

void mydeallocate(void* x, char* file, int line, int req) {
    memblock * block = (memblock*) x;
    block = block - sizeof(memblock);
    block->free = 1;
}


void initialize() {
	//char block[SYSTEM_PAGE_SIZE - sizeof(memblock)];

	memory_head = (memblock *) sbrk(SYSTEM_PAGE_SIZE - sizeof(memblock));
        DEBUG_PRINT(("Memory head address is %p\n", memory_head));
	memory_head->start = memory_head + sizeof(memblock);
	memory_head->next = NULL;
	memory_head->size = SYSTEM_PAGE_SIZE - sizeof(memblock);
	memory_head->free = 1;


	initialized = 1;
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
    block->next = (memblock *) sbrk(SYSTEM_PAGE_SIZE - sizeof(memblock));
    block = block->next;
    block->start = block + sizeof(memblock);
    block->next = NULL;
    block->size = SYSTEM_PAGE_SIZE - sizeof(memblock);
    block->free = 0;

    return block;
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
