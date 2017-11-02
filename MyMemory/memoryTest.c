#include <stdio.h>
#include <stdlib.h>

#include "my_memory.h"

int main() {

    // Malloc 0 bytes
    malloc(0);

    // Malloc > system's page size
    malloc(SYSTEM_PAGE_SIZE + 100);

    return 0;
}