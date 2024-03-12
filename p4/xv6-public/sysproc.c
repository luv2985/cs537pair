#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "wmap.h"


int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// memory mapping syscalls
uint
sys_wmap(void)
{
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
	
	return wmap(addr, length, flags, fd);
}

int
sys_wunmap(void)
{
	uint addr;
    if (arguint(0, &addr) < 0) {
        return -1;
    }

	return wunmap(addr);
}

uint
sys_wremap(void)
{
	uint oldaddr;
    int oldsize, newsize, flags;

    // Fetch integer arguments using argint
    if (arguint(0, &oldaddr) < 0 ||    // First argument
        argint(1, &oldsize) < 0 ||  // Second argument
        argint(2, &newsize) < 0 ||   // Third argument
        argint(3, &flags) < 0)        // Fourth argument
    {
        return -1; // Error handling: Return an error code
    }


	return wremap(oldaddr, oldsize, newsize, flags);
}

int
sys_getpgdirinfo(void)
{
	struct pgdirinfo* pdinfo;

    if (argptr(0, (char **)&pdinfo, sizeof(struct pgdirinfo)) < 0) {
        // printf("get pgdir info arg 0\n");
        return FAILED;
    }

	return getpgdirinfo(pdinfo);
}

int
sys_getwmapinfo(void)
{
	struct wmapinfo* wminfo;
    
    // i guess we need it?
    if (argptr(0, (char **)&wminfo, sizeof(struct wmapinfo)) < 0) {
        // printf("get wmap info arg 0\n");
        return -1;
    }

	return getwmapinfo(wminfo);
}

