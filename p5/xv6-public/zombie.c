#include "types.h"
#include "user.h"

void fn1(void* arg) {
  nice(10);
  for (int i = 0; i < 100000; i++) {
    if (i % 1000 == 0) {
      sleep(0);
    }
  }

  exit();
}

void fn2(void* arg) {
  nice(-10);
  for (int i = 0; i < 100000; i++) {
    if (i % 1000 == 0) {
      sleep(0);
    }
  }

  exit();
}

int main() {
	printf(1, "start\n");
  char* stack1 = (char*)malloc(4096);
  char* stack2 = (char*)malloc(4096);
	printf(1, "mid\n");
  clone(fn1, stack1	+ 4096, 0);
  clone(fn2, stack2 + 4096, 0);
	printf(1, "for?\n");

  for (int i = 0; i < 100000; i++) {
    if (i % 1000 == 0) {
      sleep(0);
    }
  }
	printf(1, "wait\n");
  wait();
  wait();
  exit();
}
