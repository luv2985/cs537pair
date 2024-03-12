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


// handle page fault, 0 if correct, -1 if not found

int alloc_nu_pte(struct proc* curproc, struct map_en* entry, uint va)
{

    char* mem = kalloc();
	if (mem == 0) {
		return -2;
	}
	if (entry->flags & MAP_ANONYMOUS)
	{
		// anonymous mapping
		if (mappages(curproc->pgdir, (void*) va, PGSIZE, V2P(mem), PTE_W | PTE_U) != 0) {
			kfree(mem);
			return -3;
		}
		memset(mem, 0, PGSIZE);
	} else {
		// file-backed mapping
        struct file* f = curproc->ofile[entry->fd];
        
		ilock(f->ip);
        readi(f->ip, mem, va - entry->addr, PGSIZE);
        iunlock(f->ip);
        
		if (mappages(curproc->pgdir, (void *) va, PGSIZE, V2P(mem), PTE_W | PTE_U) != 0) {
            kfree(mem);
            return -4;
        }
	}

	entry->lpgs++;
	return 0;
}

/*
int alloc_nu_map(struct proc* curproc, int start, int end, int index)
{
	for (int i = start; i < end; i += PGSIZE) {
		if (alloc_nu_pte(curproc, i, index) == -1) {
			return -1;
		}
	}
	return 0;
}*/



int pf_handler(struct proc* curproc, uint va)
{
	// check PGallign
	/*if (va % PGSIZE != 0) {
		return -5;
	}*/

	// find length
	for (int i = 0; i < 16; i++) {
		struct map_en* entry = &(curproc->wmaps[i]);
		int botaddr = entry->addr;
		int topaddr = botaddr + entry->length;
		
		if (va <= topaddr && va >= botaddr) {
			//found
			alloc_nu_pte(curproc, entry, PGROUNDDOWN(va));
			return 0;
		}
	}
	return -1;
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
int getwmapinfo(struct wmapinfo* wminfo)
{
	struct proc *curproc = myproc();
/*	struct wmapinfo *wminfo;
    
    // i guess we need it?
    if (argptr(0, (char **)&wminfo, sizeof(struct wmapinfo)) < 0) {
        // printf("get wmap info arg 0\n");
        return FAILED;
    }
*/
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
int getpgdirinfo(struct pgdirinfo* pdinfo) {
    struct proc *curproc = myproc();
/*	struct pgdirinfo *pdinfo;

    if (argptr(0, (char **)&pdinfo, sizeof(struct pgdirinfo)) < 0) {
        // printf("get pgdir info arg 0\n");
        return FAILED;
    }
*/
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
uint wmap(uint addr, int length, int flags, int fd)
{
    struct proc *curproc = myproc();
	struct file *f = 0;

	/* INPUTS */
/*	uint addr;
	int length, flags, fd;

    // Fetch integer arguments using argint
    if (arguint(0, &addr) < 0 ||    // First argument
        argint(1, &length) < 0 ||  // Second argument
        argint(2, &flags) < 0 ||   // Third argument
        argint(3, &fd) < 0)        // Fourth argument
    {
        return -1; // Error handling: Return an error code
    }
*/

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
			cme[i].flags = flags;
			cme[i].fd = fd;
			curproc->total_maps++;
			break;
        }
    }	

    return va;
}


// Implementation of munmap system call
int wunmap(uint addr)
{
    struct proc *curproc = myproc();
/*    uint addr;
    if (arguint(0, &addr) < 0) {
        return -1;
    }*/
	addr = PGROUNDDOWN(addr);

	/* CATCH ERROR */
	if (addr % PGSIZE != 0)
	{
		return FAILED;
	}

    // adjust linked list
	int free_len = 0;
    struct map_en* list = curproc->wmaps;
	int flags = 0;
	int found = 0;
	int fd = 0;

    for (int i = 0; i < 16; i++) {
        if (list[i].addr == addr) {
			free_len = list[i].length;
			flags = list[i].flags;
            fd = list[i].fd;
			list[i].valid = 0;  // Free the memory occupied by the removed node
            curproc->total_maps--;
			found = 1;
            break;
        }
    }
	if (found == 0) {
		return -1;
	}

    // Go into pg t, if page is present and valid, remove
	int anon = flags & MAP_ANONYMOUS;
	int shared = flags & MAP_SHARED;
	for (int i = addr; i < addr + free_len; i += PGSIZE) {
		pte_t *pte = walkpgdir(myproc()->pgdir, (void *) i, 0);
		if (pte != 0) {
			uint a = PTE_ADDR(*pte);
			if (!anon && shared) {
				struct file* f = curproc->ofile[fd];
				filewrite(f, P2V(a), PGSIZE);
			}

			kfree(P2V(a));		
			*pte = 0;
		}
	}
    

	return SUCCESS;
}


// Implementation of mremap system call
uint wremap(uint oldaddr, int oldsize, int newsize, int flags)
{
    // Your implementation here
	return 0;
}


