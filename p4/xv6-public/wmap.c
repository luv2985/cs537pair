// wmap.c

#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "fcntl.h"
#include "defs.h"
#include "memlayout.h"
#include "wmap.h"


#define USERBOUNDARY 0x60000000
#define KERNBASE 0x80000000

/********** HELPER METHODS ***********/

// checks if the addr in pg t is valid; 0 if yes, -1 if no
int check_valid(uint addr)
{
    pte_t *pte = walkpgdir(myproc()->pgdir, (void *)addr, 0);

    if (pte != 0 && (*pte & PTE_P) != 0) {
        // The page is present and valid
        return SUCCESS;
    } else {
        // The page is not present or not valid
        return FAILED;
    }
}

int find_nu_addr(uint va)
{
    char* mem = kalloc();
    mappages(myproc()->pgdir, (void*) va, PGSIZE, V2P(mem), PTE_W | PTE_U);
    return 0;
}


/*********** INFO FUNCTIONS ***********/

/*
 *
 */
int sys_getwmapinfo(struct wmapinfo *wminfo)
{
	


    return 0;
}




/*********** MAIN FUNCTIONS ***********/


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
uint sys_wmap(uint addr, int length, int flags, int fd)
{
    
	/* CHECK FLAGS */

	// check length
	if(length <= 0) {
        return -5;
		//return FAILED;
    }

	// check flags
    if ((flags & MAP_SHARED) && (flags & MAP_PRIVATE)) {
        return -2;
		//return FAILED;
    }

    // get process
    //struct proc *curproc = myproc();
    uint va;

	// MAP_FIXED flag
    if (flags & MAP_FIXED) {
        if (addr < USERBOUNDARY || addr >= KERNBASE || addr % PGSIZE != 0) {
            return -3;
			//return FAILED;
        }
        // Check if the specified address range is available x60000000
        if(check_valid(addr)<0) {
            return -4;
			//return FAILED;
        }

		va = addr;

		/*
        // valid, do lazy alloc
        if (find_nu_addr(addr) != 0) {
            printf("lazy alloc f\n");
            return FAILED;
        }
        // TODO: update pg t

        return addr;*/
    } else {
		/*
        int iter = length/PGSIZE;
        int num_pages = 0; // keep record of number of pages created cuz max is 16
        // loop thru pg t to get available space
        for(int i=0; i<iter; i++) {
            va = USERBOUNDARY + i * PGSIZE;
            // if success, return 0, so loop stops; otherwise keep looping
            while(check_valid(va)) {
                va += PGSIZE;
                if(va >= KERNBASE) {
                    printf("va > kern\n");
                    return FAILED;
                }
            }            
        }
        // yay it worked now do lazy alloc
        if (find_nu_addr(addr) != 0) {
            printf("lazy alloc f\n");
            return FAILED;
        }
        // TODO: update pg t

        num_pages++;
        // check if surpass 16 pages
        if(num_pages > 16) {
            printf("too many pages\n");
            return FAILED;
        } */
		return -6;
	}
    

	if(flags & MAP_ANONYMOUS) {
            // we don't do anything?
    } else if (flags & MAP_SHARED) {
            // TODO: Implement shared mapping logic
            // Copy mappings from parent to child

    } else if (flags & MAP_PRIVATE) {
            // TODO: Implement private mapping logic
            // Copy mappings from parent to child, use different physical pages
    }
    
	/* LAZY ALLOCATION */
	uint nva = va;	
	int leftover = length;
	while (leftover > 0) {
		//growproc(PGSIZE); // do we need to grow the process size? or is this handelled elsewhere?
		// allocate new pages
		find_nu_addr(va);

		// advance iter
		nva += PGSIZE;
		leftover = leftover - PGSIZE;
	}

	// TODO: update process size: myproc()->sz += length or something

    return va;
}


// Implementation of munmap system call
int sys_wunmap(uint addr)
{
	/* CATCH ERROR */
	if (addr % PGSIZE != 0)
	{
		return FAILED;
	}

	struct proc* currproc = myproc();

	/* FREE */
	pte_t* entry = walkpgdir(currproc->pgdir, (void*)&addr, 0);
	uint physical_address = PTE_ADDR(*entry);
	kfree(P2V(physical_address));


	return SUCCESS;
}



/*
// Implementation of mremap system call
void *wremap(void *old_address, size_t old_size, size_t new_size, int flags)
{
    // Your implementation here
}

*/
