// Mutex implementation

#include "types.h"
#include "defs.h"
#include "param.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"
#include "mutex.h"

void
macquire(mutex *m)
{
  acquire(&m->lk);
  while (m->locked) {
    sleep(lk, &m->lk);
  }
  m->locked = 1;
  m->pid = myproc()->pid;
  release(&m->lk);
}

void
mrelease(mutex *m)
{
  acquire(&m->lk);
  m->locked = 0;
  m->pid = 0;
  wakeup(m);
  release(&m->lk);
}

