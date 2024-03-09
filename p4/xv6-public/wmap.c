// wmap.c

#include "stdio.h"
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
#include "memlayout.h"
#include "wmap.h"

#define USERBOUNDARY 0x60000000
#define KERNBASE 0x80000000

/********** HELPER METHODS ***********/

// checks if the addr in pg t is valid; 0 if yes, -1 if no
int check_valid(uint addr)
{
    pte_t *pte = walkpgdir(myproc()->pgdir, (void *)addr, 0);

    if (pte == 0) {
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

// count num pages allocated by a map
int count_allocated_pages(struct proc *curproc, uint addr, int length) {
    int count = 0;
    uint va = addr;

    while (length > 0) {
        pte_t *pte = walkpgdir(curproc->pgdir, (void *)va, 0);
        if (pte != 0 && (*pte & PTE_P) != 0) {
            count++;
        }
        va += PGSIZE;
        length -= PGSIZE;
    }

    return count;
}



/*********** INFO FUNCTIONS ***********/

/*
 * 
 */
int getwmapinfo(void)
{
	struct proc *curproc = myproc();
	struct wmapinfo *wminfo;
    
    // i guess we need it?
    if (argptr(0, (char **)&wminfo, sizeof(struct wmapinfo)) < 0) {
        // printf("get wmap info arg 0\n");
        return FAILED;
    }

    wminfo->total_mmaps = 0;

    struct wmapnode *node = curproc->wmaps.head;
    int i = 0;

    // iterate
    while (node && i < MAX_WMMAP_INFO) {
        wminfo->addr[i] = node->addr;
        wminfo->length[i] = node->length;
        wminfo->n_loaded_pages[i] = node->n_loaded_pages;

        node = node->next;
        i++;
    }

    wminfo->total_mmaps = i;

    return 0;
}

/* 
 *
 */
int getpgdirinfo(void) {
    struct proc *curproc = myproc();
	struct pgdirinfo *pdinfo;

    if (argptr(0, (char **)&pdinfo, sizeof(struct pgdirinfo)) < 0) {
        // printf("get pgdir info arg 0\n");
        return FAILED;
    }

    pdinfo->n_upages = 0;

    // or should we also do a linked list?
    for (int i = 0; i < MAX_UPAGE_INFO; i++) {
        pdinfo->va[i] = 0;
        pdinfo->pa[i] = 0;
    }

    pde_t *pgdir = curproc->pgdir;
    uint va = 0;
    int i = 0;

    while (i < MAX_UPAGE_INFO) {
        pte_t *pte = walkpgdir(pgdir, (void *)va, 0);

        if (pte != 0 && (*pte & PTE_P) != 0 && (*pte & PTE_U) != 0) {
            pdinfo->va[i] = va;
            pdinfo->pa[i] = PTE_ADDR(*pte);
            pdinfo->n_upages++;
            i++;
        }

        va += PGSIZE;
    }

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
//uint wmap(uint addr, int length, int flags, int fd)
uint wmap(void)
{
    struct proc *curproc = myproc();

	/* INPUTS */
	uint addr;
	int length, flags, fd;

    // Fetch integer arguments using argint
    if (arguint(0, &addr) < 0 ||    // First argument
        argint(1, &length) < 0 ||  // Second argument
        argint(2, &flags) < 0 ||   // Third argument
        argint(3, &fd) < 0)        // Fourth argument
    {
        return -1; // Error handling: Return an error code
    }

	return addr;

	/* CHECK FLAGS */
	
	// check length
	if((uint)length <= 0) {
        return PGSIZE;
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
            return -5;
			//return FAILED;
        }
        // Check if the specified address range is available x60000000
        if(check_valid(addr)<0) {
            return -4;
			//return FAILED;
        }

		va = addr;

    } else {
		//find suitable address
		
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
                    // printf("va > kern\n");
                    return FAILED;
                }
            }            
        }
        // yay it worked now do lazy alloc
        if (find_nu_addr(addr) != 0) {
            // printf("lazy alloc f\n");
            return FAILED;
        }

        num_pages++;
        // check if surpass 16 pages
        if(num_pages > 16) {
            // printf("too many pages\n");
            return FAILED;
        } */
		return -6;
	}
    

	if(flags & MAP_ANONYMOUS) {
        // load pages? if not anonymous
		
    }

	if (flags & MAP_SHARED) {
            // TODO: Implement shared mapping logic
            // Copy mappings from parent to child

    } else if (flags & MAP_PRIVATE) {
            // TODO: Implement private mapping logic
            // Copy mappings from parent to child, use different physical pages
    }
    

	/* LAZY ALLOCATION */
	uint nu_va = va;
	for (int leftover = length; leftover > 0; leftover -= PGSIZE) {
		//growproc(PGSIZE); // do we need to grow the process size? or is this handelled elsewhere?
		// allocate new pages
		find_nu_addr(va);

		// advance iter
		nu_va += PGSIZE;
	}


    /* UPDATE MAP TRACKER */
	struct wmapnode *new_node = (struct wmapnode *)kalloc();
    // do we really need to check if new_node exists?
    if (new_node != 0) {
        new_node->addr = va;
        new_node->length = length;
        new_node->n_loaded_pages = count_allocated_pages(curproc, addr, length);
        // shove it in
        new_node->next = curproc->wmaps.head;
        new_node->prev = 0;
        if (curproc->wmaps.head != 0) {
            curproc->wmaps.head->prev = new_node;  // Update prev pointer of the current head
        }
        curproc->wmaps.head = new_node;
        curproc->wmaps.total_mmaps++;
    }


    return va;
}


// Implementation of munmap system call
int wunmap(uint addr)
{
    struct proc *curproc = myproc();
	/* CATCH ERROR */
	if (addr % PGSIZE != 0)
	{
		return FAILED;
	}

	struct proc* currproc = myproc();

    // adjust linked list
	int free_len = 0;
    struct wmapnode *node = curproc->wmaps.head;
    while (node) {
        if (node->addr == addr) {
            if (node->prev) {
                node->prev->next = node->next;
            } else {
                curproc->wmaps.head = node->next;
            }

            if (node->next) {
                node->next->prev = node->prev;
            }
			
			free_len = node->length;

            kfree((char*)node);  // Free the memory occupied by the removed node
            curproc->wmaps.total_mmaps--;
            break;
        }

        node = node->next;
    }


	uint iter_addr = addr;
	for (int i = free_len; i > 0; i -= PGSIZE) {
		/* FREE */
		pte_t* entry = walkpgdir(currproc->pgdir, (void*)&iter_addr, 0);
		uint physical_address = PTE_ADDR(*entry);
		kfree(P2V(physical_address));
		iter_addr += PGSIZE;
	}

	return SUCCESS;
}




// Implementation of mremap system call
uint wremap(uint oldaddr, int oldsize, int newsize, int flags)
{
    // Your implementation here
	return 0;
}


