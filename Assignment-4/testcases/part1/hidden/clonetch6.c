#include<ulib.h>
#include<gthread.h>
/*thread functions must be declared as static*/

struct arg{
    unsigned id;
    unsigned sleep;
};
static void *thfunc1(void *ar){
            struct arg * ag = (struct arg*)ar;
            if(ag->id == 0){
                sleep(ag->sleep);
            }
            else if(ag->id == 1){
               sleep(ag->sleep);
            }
            exit(0);
}

/* this test case creates 4 threads, sleeps and allows 2 thread to finish, then create two more threads*/

int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{
  int ctr=0;
  struct arg ag[4];
  void *stackp[4];
  int th_pid[4];
  int tharg[4];

  for(ctr=0; ctr<4; ++ctr){
       stackp[ctr] = mmap(NULL, 8192, PROT_READ|PROT_WRITE, 0);   // min stack size 8192.
       if(!stackp[ctr] || stackp[ctr] == MAP_ERR){
            printf("can not allocated stack\n");
            exit(0);
       }
       ag[ctr].id = ctr;
       ag[ctr].sleep = 20;
       th_pid[ctr] = clone(&thfunc1, ((u64)stackp[ctr] + 8192), &ag[ctr]);   // returns the pid of the thread
       if(th_pid[ctr] <= 0){
          printf("error creating thread!\n");
          exit(0);
       }
       make_thread_ready(th_pid[ctr]);
       printf("created thread\n");
  }
  sleep(5);    // 2 threads sleeps for max 20 units, let us sleep for 5 units to finish 2 threads
  for(ctr=0; ctr<2; ++ctr){
       stackp[ctr] = mmap(NULL, 8192, PROT_READ|PROT_WRITE, 0);   // min stack size 8192.
       if(!stackp[ctr] || stackp[ctr] == MAP_ERR){
            printf("can not allocated stack\n");
            exit(0);
       }
       th_pid[ctr] = clone(&thfunc1, ((u64)stackp[ctr] + 8192), &ag[ctr]);   // returns the pid of the thread
       if(th_pid[ctr] <= 0){
          printf("error creating thread!\n");
          exit(0);
       }
       make_thread_ready(th_pid[ctr]);
       printf("created thread\n");
  }

  sleep(50);    // threads sleeps for max 20 units, let us sleep for 50 units
  return 0;
}
