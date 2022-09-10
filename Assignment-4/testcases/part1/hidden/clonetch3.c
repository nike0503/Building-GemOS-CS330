#include<ulib.h>
#include<gthread.h>
/* XXX  Do not declare any global/static vars for this assignment*/

/*Thread functions must be declared as static*/
struct argument{
    unsigned num;
    unsigned long result;
};

static void * thsum(void *arg){
    int i;
    struct argument *ag = (struct argument*)arg;
    for(i=1; i<=ag->num; i++){
         ag->result += i;
    }
    exit(0);
}

static void * thmul(void *arg){
    int i;
    struct argument *ag = (struct argument*)arg;
    for(i=1; i<=ag->num; i++){
         ag->result *= i;
    }
    exit(0);
}
/* Test case to check whether the stack passed is handled correctly*/

int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{
  void *stackp;
  int thpid[2];
  struct argument tharg[2];
  tharg[0].num = 15;
  tharg[0].result = 0;
  tharg[1].num = 11;
  tharg[1].result = 1;
  
  stackp = mmap(NULL, 8192, PROT_READ|PROT_WRITE, 0);
  if(!stackp || stackp == MAP_ERR){
        printf("Can not allocated stack\n");
        exit(0);
  }
  thpid[0] = clone(&thsum, ((u64)stackp) + 4096, (void *)&tharg[0]);   // Returns the PID of the thread
  if(thpid[0] <= 0){
       printf("Error creating thread!\n");
       exit(0);
  }

  thpid[1] = clone(&thmul, ((u64)stackp) + 8192, (void *)&tharg[1]);   // Returns the PID of the thread
  if(thpid[1] <= 0){
       printf("Error creating thread!\n");
       exit(0);
  }
  printf("Created threads\n");
  make_thread_ready(thpid[0]);
  make_thread_ready(thpid[1]);
  
  sleep(50);    // lets sleep for the thread to finish
  printf("In parent: \n");
  printf("Thread 1 result: %u\n",tharg[0].result);
  printf("Thread 2 result: %u\n",tharg[1].result);
  printf("\n");
  return 0;
}
