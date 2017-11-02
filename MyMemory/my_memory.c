#include "my_memory.h"

#define DEBUG_PRINT(x) printf x

// Ptr for start of memory
void * memory_head;

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
    if (size > SYSTEM_PAGE_SIZE - sizeof(meta_data)) {
        DEBUG_PRINT(("Requested size is greater than system's page size, returning null\n"));
        return NULL;
    }

    return NULL;
}

void mydeallocate(void* x, char* file, int line, int req) {
    DEBUG_PRINT(("mydeallocate called from %d\n", req));
}

void initialize() {
    DEBUG_PRINT(("Initializing memory space\n"));
    DEBUG_PRINT(("System page size: %lu\n", SYSTEM_PAGE_SIZE));


    initialized = 1;
}