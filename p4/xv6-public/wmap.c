// wmap.c

#include "types.h"
#include "defs.h"
#include "wmap.h"
#include "mmu.h"
#include "proc.h"

#define USERBOUNDARY 0x60000000
#define KERNBASE 0x80000000

// checks if the addr in pg t is valid; 0 if yes, -1 if no
int check_valid(uint addr) {
    pte_t *pte = walkpgdir(myproc()->pgdir, (void *)addr, 0);

    if (pte != 0 && (*pte & PTE_P) != 0) {
        // The page is present and valid
        return 0;
    } else {
        // The page is not present or not valid
        return -1;
    }
}

int find_nu_addr() {
    char* mem = kalloc();
    mappages(myproc()->pgdir, USERBOUNDARY, 4096, V2P(mem), PTE_W | PTE_U);
}

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
uint sys_wmap(uint addr, int length, int flags, int fd) {
    // get process
    struct proc *curproc = myproc();
    uint va;

    if(length <= 0) {
        printf("u dumb\n");
        return -1;
    }

	// check flags
    if ((flags & MAP_SHARED) && (flags & MAP_PRIVATE)) {
        printf("flags collide\n");
        return -1;
    }

    if (flags & MAP_FIXED) {
        if (addr < USERBOUNDARY || addr >= KERNBASE || addr % PGSIZE != 0) {
            printf("addr f\n");
            return -1;
        }
        // Check if the specified address range is available
        if(check_valid(addr)<0) {
            printf("fixed addr f\n");
            return -1;
        }
    } else {
        printf("not map fix\n");
        return -1;
    }

    int iter = length/PGSIZE;
	// track allocated physical addresses: loop through and keep getting physical address, add to page table
	for(int i=0; i<iter; i++) {
        find_nu_addr(va);
        va += PGSIZE;
    }
    
}

/*
// Implementation of munmap system call
int wunmap(void *addr, size_t length) {
    // Your implementation here
}

// Implementation of mremap system call
void *wremap(void *old_address, size_t old_size, size_t new_size, int flags) {
    // Your implementation here
}

*/