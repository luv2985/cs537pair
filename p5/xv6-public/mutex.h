// the mutex header file


typedef struct {
  uint locked;       // Is the lock held?

  // For debugging:
  int pid;           // Process holding lock
} mutex;

