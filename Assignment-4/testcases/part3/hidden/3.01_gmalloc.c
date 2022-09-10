#include<ulib.h>
#include<gthread.h>
/*Thread functions must be declared as static*/

static void *thfunc(void *arg){
       void *lptr;      
       u64 *uptr = (u64 *)arg;
       lptr = gmalloc(8192, GALLOC_OTRDWR);
       if(!lptr){
                printf("galloc failed %d\n", getpid());
                return NULL;
       }
       uptr[0] = (u64)lptr; 
       *((u64 *) lptr) = 0x987654321;
       *((u64 *) lptr + 512) = 0x123456789;
       sleep(50);
       gfree(lptr); 
       return NULL;
}

int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{ 
  unsigned long *ptr;
  int tid;
  u64 maps[4];
  if(gthread_create(&tid, thfunc, &maps[0]) < 0){
             printf("gthread_create failed\n");
             exit(-1);
  }
  printf("Created thread\n");
  sleep(25);
  ptr = (unsigned long *)maps[0];
  printf("page1 val = %x page2 val = %x\n", *ptr, *(ptr +512));
  gthread_join(tid);
  printf("Thread 1 has joined\n");
  return 0;
}
