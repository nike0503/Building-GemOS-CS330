#include<ulib.h>
#include<gthread.h>
/* XXX  Do not declare any global/static vars for this assignment*/

/*Thread functions must be declared as static*/

struct arg{
    unsigned id;
    int * marea;
};
static void *thfunc1(void *ar){
            struct arg *ptr = (struct arg *) ar;
            if(ptr->id == 0){
                ptr->marea[0] = 100;
            }
            if(ptr->id == 1){
                ptr->marea[1] = 200;
            }
            printf("\n");
            exit(0);
}

/* This test case mmap area in parent and both threads access it*/

int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{
  int * marea;
  void *stackp[2];
  int thpid[2];
  struct arg ag[2];
  
  marea = (int*)mmap(NULL, 4096, PROT_READ|PROT_WRITE, 0);
  if(!marea || marea == MAP_ERR){
        printf("Can not mmap\n");
        exit(0);
  }
  
  for(int i=0; i<2; i++){
    ag[i].id = i;
    ag[i].marea = marea;
    stackp[i] = mmap(NULL, 8192, PROT_READ|PROT_WRITE, 0);
    if(!stackp[i] || stackp[i] == MAP_ERR){
        printf("Can not allocated stack\n");
        exit(0);
    }
    thpid[i] = clone(&thfunc1, ((u64)stackp[i]) + 8192, (void *)&ag[i]);   // Returns the PID of the thread
    if(thpid[i] <= 0){
       printf("Error creating thread!\n");
       exit(0);
    }
  }
  printf("Created thread \n");
  make_thread_ready(thpid[0]);
  make_thread_ready(thpid[1]);
  sleep(50);    // lets sleep for the thread to finish
  printf("In parent: \n");
  printf("Thread 1 set value : %d\n",marea[0]);
  printf("Thread 2 set value : %d\n",marea[1]);
  printf("\n");
  return 0;
}
