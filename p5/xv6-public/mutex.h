// the mutex header file

#ifndef __MUTEX_H__
#define __MUTEX_H__

#include "spinlock.h"

typedef struct {
  uint locked;       // Is the lock held?
  struct spinlock lk;
  // For debugging:
  int pid;           // Process holding lock
} mutex;


//void m_init(mutex* m);

#endif
