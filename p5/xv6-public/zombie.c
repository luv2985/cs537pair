#include "types.h"
#include "user.h"

mutex m;

void fn1(void* arg) {
  sleep(0);
  nice(-10);
  macquire(&m);
  for (int i = 0; i < 100; i++) {
    if (i % 10 == 0) {
      // sleep(0) is equivalent to yield(). This gives the scheduler more chances to do scheduling
      sleep(0);
    }
  }
  printf(1, "fn1\n");
  mrelease(&m);
  exit();
}

void fn2(void* arg) {
  sleep(0);
  nice(-5);
  macquire(&m);
  for (int i = 0; i < 100; i++) {
    if (i % 10 == 0) {
      sleep(0);
    }
  }
  printf(1, "fn2\n");
  mrelease(&m);

  exit();
}

void fn3(void* arg) {
  nice(1);
  macquire(&m);
  sleep(1);
  for (int i = 0; i < 100; i++) {
    if (i % 10 == 0) {
      sleep(0);
    }
  }
  printf(1, "fn3\n");
  mrelease(&m);
  exit();
}

int main() {
  char* stack1 = (char*)malloc(4096);
  char* stack2 = (char*)malloc(4096);
  char* stack3 = (char*)malloc(4096);


  clone(fn1, stack1 + 4096, 0);
  printf(1, "1\n");
  clone(fn2, stack2 + 4096, 0);
  printf(1, "2\n");
  clone(fn3, stack3 + 4096, 0);
  printf(1, "3\n");
  sleep(1);
  printf(1, "4\n");
  nice(-7);
  printf(1, "5\n");
  sleep(0);
  for (int i = 0; i < 100000; i++) {
    if (i % 1000 == 0) {
      sleep(0);
    }
  }

  printf(1, "6\n");

  wait();
  wait();
  wait();
  exit();
}
