#include<ulib.h>
#include<gthread.h>
/*Thread functions must be declared as static*/

static void *thfunc(void *arg){
       void *lptr;      
       u64 *uptr = (u64 *)arg;
       int pid = getpid();
       if(pid == 2){
          for(int ctr=0; ctr<4; ++ctr){
              u64 tmp;
              lptr = gmalloc(8192, GALLOC_OTRDWR);
              if(!lptr){
                printf("galloc failed %d\n", getpid());
                return NULL;
              }
              uptr[ctr] = (u64)lptr; 
          }
          sleep(25);
          for(int ctr=4; ctr<8; ctr++){
               unsigned long *pTR = (unsigned long *)uptr[ctr];
               *pTR  = ctr;
               *(pTR + 512) = ctr;
          }
       }else{
          for(int ctr=4; ctr<8; ++ctr){
              u64 tmp;
              lptr = gmalloc(8192, GALLOC_OTRDWR);
              if(!lptr){
                printf("galloc failed %d\n", getpid());
                return NULL;
              }
              uptr[ctr] = (u64)lptr; 
          }
          sleep(25);
          for(int ctr=0; ctr<4; ctr++){
               unsigned long *pTR = (unsigned long *)uptr[ctr];
               *pTR  = ctr;
               *(pTR + 512) = ctr;
          }
       }  
       sleep(50);
       if(pid == 2){
          for(int ctr=0; ctr<4; ++ctr){
              if(gfree((void *)uptr[ctr]))
                  printf("Error: gfree\n");
          } 
       }else{
          for(int ctr=4; ctr<8; ++ctr){
              if(gfree((void *)uptr[ctr]))
                  printf("Error: gfree\n");
          } 
       }
       return NULL;
}

int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{ 
  unsigned long *ptr;
  int tid[2];
  u64 maps[8];
  if(gthread_create(&tid[0], thfunc, &maps[0]) < 0){
             printf("gthread_create failed\n");
             exit(-1);
  }
  printf("Created thread \n");
  if(gthread_create(&tid[1], thfunc, &maps[0]) < 0){
             printf("gthread_create failed\n");
             exit(-1);
  }
  printf("Created thread\n");
  sleep(50);
  
  for(int ctr=0; ctr<8; ctr++){
      ptr = (unsigned long *)maps[ctr];
      printf("page1 val = %x page2 val = %x\n", *ptr, *(ptr +512));
  }
  gthread_join(tid[0]);
  gthread_join(tid[1]);
  return 0;
}
