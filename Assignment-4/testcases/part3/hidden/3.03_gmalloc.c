#include<ulib.h>
#include<gthread.h>
/*Thread functions must be declared as static*/

static void *thfunc(void *arg){
       void *lptr;      
       u64 *uptr = (u64 *)arg;
       for(int ctr=0; ctr<4; ++ctr){
          u64 tmp;
          lptr = gmalloc(8192, GALLOC_OTRDWR);
          if(!lptr){
                printf("galloc failed %d\n", getpid());
                return NULL;
          }
         uptr[ctr] = (u64)lptr; 
         *((u64 *) lptr) = 0x987654321 + ctr;
         *((u64 *) lptr + 512) = 0x123456789 + ctr;
         
         tmp = *((u64 *) lptr);
         *((u64 *) lptr) = *((u64 *) lptr + 512);
         *((u64 *) lptr + 512) = tmp;

       }
       sleep(50);
       for(int ctr=0; ctr<2; ++ctr)
            gfree((void *)uptr[ctr]); 
       
       for(int ctr=0; ctr<2; ++ctr){
          u64 tmp;
          lptr = gmalloc(8192, GALLOC_OTRDWR);
          if(!lptr){
                printf("galloc failed %d\n", getpid());
                return NULL;
          }
         uptr[ctr] = (u64)lptr; 
         *((u64 *) lptr) = 0xABCD4321 + ctr;
         *((u64 *) lptr + 512) = 0x1234ABCD + ctr;
         
         tmp = *((u64 *) lptr);
         *((u64 *) lptr) = *((u64 *) lptr + 512);
         *((u64 *) lptr + 512) = tmp;

       }
       sleep(50);
       for(int ctr=0; ctr<4; ++ctr)
            gfree((void *)uptr[ctr]); 
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
  for(int ctr=0; ctr<4; ctr++){
      ptr = (unsigned long *)maps[ctr];
      printf("page1 val = %x page2 val = %x\n", *ptr, *(ptr +512));
  }
  sleep(75);
  for(int ctr=0; ctr<4; ctr++){
      ptr = (unsigned long *)maps[ctr];
      printf("page1 val = %x page2 val = %x\n", *ptr, *(ptr +512));
  }
  gthread_join(tid);
  printf("Thread 1 has joined\n");
  return 0;
}
