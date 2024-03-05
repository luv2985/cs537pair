// mmap.c

#include "types.h"
#include "defs.h"
#include "mmap.h"

/*
 * EDIT: functionality of system call wmap goes here
 * Iteratively make calls to physical memory (per page) to obtain addresses to add to the page table of a specific process
 * implementation is considered "lazy". 16 max mem allocs.
 *
 * inputs:
 *  uint addr       virtual address
 *  int length      length of mapping in bytes
 *  int flags       mm flags
 *  int fd          file descriptor 
 *
 * flags:
 *  MAP_ANONYMOUS   not file-backed mapping, ignore fd
 *  MAP_SHARED      mapping is shared, copy addresses from parent to child
 *  MAP_PRIVATE     mapping is not shared, copy addresses from parent to child, but map the addresses to new physical addresses
 *  MAP_FIXED       if set, input addr must be the exact virtual address, else find a page in virtual mem from x60000000 and x80000000
 *
 * outputs:
 *  return
 *  
 */
intn sys_wmap(uint addr, int length, int flags, int fd)
{
	// check flags

	// check if fixed addr is alr allocated

	// track allocated physical addresses: loop through and keep getting physical address, add to page table
	
    // 
}

// Implementation of munmap system call
int munmap(void *addr, size_t length) {
    // Your implementation here
}

// Implementation of mremap system call
void *mremap(void *old_address, size_t old_size, size_t new_size, int flags) {
    // Your implementation here
}

