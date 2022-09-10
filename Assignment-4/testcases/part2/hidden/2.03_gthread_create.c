#include<ulib.h>
#include<gthread.h>
/*Thread functions must be declared as static*/
static void *thfunc(void *arg){
            int *ptr = (int *) arg;
            int iter = *ptr;
            for(int ctr=0; ctr < iter; ++ctr){
               (*ptr)++;
            } 
            return ptr;
}

int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{ 
  int s_count[8];
  int tid[8];
  int ctr, ctr1;
  void *retval;
 
  for(ctr1=0; ctr1<4; ++ctr1){ 
      for(ctr=0; ctr<4; ++ctr){
        s_count[ctr] = ctr + ctr1 + 1;
        if(gthread_create(&tid[ctr], thfunc, &s_count[ctr]) < 0){
             printf("gthread_create failed\n");
             exit(-1);
        }
	printf("Created thread:\n");
      }        
     sleep(5);
     sleep(5);
     sleep(5);
     sleep(5);
     for(ctr=0; ctr<4; ++ctr){
        retval = gthread_join(tid[ctr]);
        printf("Thread returned is %d\n",*((int *)retval));
     }
  }
  return 0;
}
