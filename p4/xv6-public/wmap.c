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

// checks if the addr in pg t is valid; 0 if yes, failed ending address if no, -1 if empty
int check_valid(uint addr, int length)
{
	struct proc* curproc = myproc();
	for (int i = 0; i < 16; i++) {
		struct map_en* item = &(curproc->wmaps[i]);
		if (item->valid == 1) {
			int botaddr = item->addr;
			int topaddr = botaddr + item->length;
			int addrlen = addr + length;

			if ((addr >= botaddr && addr < topaddr) ||
				(addrlen >= botaddr && addrlen < topaddr) ||
				(botaddr >= addr && botaddr < addrlen) ||
				(topaddr >= addr && topaddr < addrlen)) {
				return PGROUNDUP(topaddr);
			}
		}
	}
	return 0;
}


/*
int find_nu_addr(uint va)
{
    uint mem = (uint) kalloc();
	if (mem == 0) { return -1; }

    mappages(myproc()->pgdir, (void*) va, PGSIZE, V2P(mem), PTE_W | PTE_U);
    return 0;
}*/

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

    wminfo->total_mmaps = curproc->total_maps;

	struct map_en* list = curproc->wmaps;

    // iterate
    for (int i = 0; i < 16; i++) {
		if (list[i].valid == 1) {
			wminfo->addr[i] = list[i].addr;
			wminfo->length[i] = list[i].length;
			wminfo->n_loaded_pages[i] = list[i].lpgs;
		}
    }
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
	struct file *f = 0;

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


	/* CHECK FLAGS */
	
	// check length
	if ((uint)length <= 0) {
        return PGSIZE;
		//return FAILED;
    }

	// check flags
    if ((flags & MAP_SHARED) && (flags & MAP_PRIVATE)) {
        return -2;
		//return FAILED;
    }

    uint va = 0;

	// MAP_FIXED flag
    if (flags & MAP_FIXED) {
        if (addr < USERBOUNDARY || addr >= KERNBASE || addr % PGSIZE != 0) {
            return -3;
			//return FAILED;
        }
        // Check if the specified address range is available x60000000
        if(check_valid(addr, length) != 0) {
            return -1;
			//return FAILED;
        }

		va = addr;

    } else {
        if(curproc->total_maps > 15) {
            return FAILED;
        }

		int found = 0;
        // loop thru pg t to get available space
		int t_va = USERBOUNDARY;
		while(t_va + length < KERNBASE) {
            int valid = check_valid(t_va, length);
			if (valid == 0) {
				va = t_va;
				found = 1;
				break;
			}
			t_va = valid;
		}
		if (!found)
			return -7;
	}
    

    // we dont do anything extra for this?
    if(flags & MAP_ANONYMOUS) {
        // do nothing
    } else {
		if (fd <= 0)
		{
			return -5;
		}

		f = filedup(curproc->ofile[fd]);
		if (f == 0)
		{
			return -6;
		}
	}
	
/*
	if (flags & MAP_SHARED) {
        // TODO: Implement shared mapping logic
        // Copy mappings from parent to child
        struct proc *parent = curproc->parent;
        struct wmapnode *parent_node = parent->wmaps.head;
        
        // go thru all parent's nodes
        while (parent_node) {
            if (parent_node->addr >= addr && parent_node->addr < addr + length) {
                // This part of the parent's mapping overlaps with the new mapping
                uint nu_va = va + (parent_node->addr - addr);
                
                // TODO: Copy the mapping from the parent to the child
                // You need to map the same physical pages to the new virtual address in the child process
                if (find_nu_addr(nu_va) != 0) { 
                    return -7;
                }

                pte_t *parent_pte = walkpgdir(parent->pgdir, (void *)parent_node->addr, 0);
                uint parent_pa = PTE_ADDR(*parent_pte);
                mappages(curproc->pgdir, (void *)nu_va, PGSIZE, parent_pa, PTE_W | PTE_U);
            }

            parent_node = parent_node->next;
        }
    } else if (flags & MAP_PRIVATE) {
            // TODO: Implement private mapping logic
            // Copy mappings from parent to child, use different physical pages
    }
    
    if(!(flags & MAP_SHARED)) {
        
    }
	LAZY ALLOCATION
        uint nu_va = va;
        for (int leftover = length; leftover > 0; leftover -= PGSIZE) {
            //growproc(PGSIZE); // do we need to grow the process size? or is this handelled elsewhere?
            // allocate new pages
            if (find_nu_addr(nu_va) == -1) { return -8;}

            // advance iter
            nu_va += PGSIZE;
        }*/



    /* UPDATE MAP TRACKER */
	struct map_en* cme = curproc->wmaps;	

	for (int i = 0; i < 16; i++)
    {
        if (cme[i].valid == 0)
        {
            // place it here
            cme[i].valid = 1;
			cme[i].addr = va;
			cme[i].length = length;
			cme[i].lpgs = 0;
			curproc->total_maps++;
			break;
        }
    }	

    return va;
}


// Implementation of munmap system call
int wunmap(void)
{
    struct proc *curproc = myproc();
    uint addr;
    if (arguint(0, &addr) < 0) {
        return -1;
    }
	/* CATCH ERROR */
	if (addr % PGSIZE != 0)
	{
		return FAILED;
	}

    // adjust linked list
	int free_len = 0;
    free_len ++;
    free_len --;
    struct map_en* list = curproc->wmaps;

    for (int i = 0; i < 16; i++) {
        if (list[i].addr == addr) {
			free_len = list[i].length;

            list[i].valid = 0;  // Free the memory occupied by the removed node
            curproc->total_maps--;
            break;
        }
    }

    // Go into pg t, if page is present and valid, remove
    pte_t *pte = walkpgdir(myproc()->pgdir, (void *)addr, 0);
    if (pte != 0 && (*pte & PTE_P)) {
        uint a = PTE_ADDR(*pte);
        kfree((char *)P2V(a));
        *pte = 0;
    } 

    /* 
    // i dont think we need to iter thru
	uint iter_addr = addr;
	for (int i = free_len; i > 0; i -= PGSIZE) {
		// FREE
		pte_t* entry = walkpgdir(currproc->pgdir, (void*)&iter_addr, 0);
		uint physical_address = PTE_ADDR(*entry);
		kfree(P2V(physical_address));
		iter_addr += PGSIZE;
	}
    */

	return SUCCESS;
}


// Implementation of mremap system call
uint wremap(uint oldaddr, int oldsize, int newsize, int flags)
{
    // Your implementation here
	return 0;
}


