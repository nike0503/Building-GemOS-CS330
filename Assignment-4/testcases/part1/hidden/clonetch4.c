#include<ulib.h>
#include<gthread.h>
/*Thread functions must be declared as static*/
static void *thfunc1(void *arg){
            unsigned long *ptr = (unsigned long *) arg;
            *ptr = (unsigned long)mmap(NULL, 8192, PROT_READ|PROT_WRITE, 0);
            exit(0);
}

/*single thread allocates mmap and parent accesses it*/

int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{
  void *stackp;
  int thpid;
  unsigned long tharg;
  
  stackp = mmap(NULL, 8192, PROT_READ|PROT_WRITE, 0);
  if(!stackp || stackp == MAP_ERR){
        printf("Can not allocated stack\n");
        exit(0);
  }
  tharg = 10;
  thpid = clone(&thfunc1, ((u64)stackp) + 8192, &tharg);   // Returns the PID of the thread
  if(thpid <= 0){
       printf("Error creating thread!\n");
       exit(0);
  }
  make_thread_ready(thpid);
  printf("Created thread\n");
  sleep(20); 
  *(int*)tharg = 10;
  return 0;
}
