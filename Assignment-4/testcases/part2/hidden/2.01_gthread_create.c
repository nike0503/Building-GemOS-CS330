#include<ulib.h>
#include<gthread.h>
/*Thread functions must be declared as static*/
static void *thfunc(void *arg){
            int *ptr = (int *) arg;
            int iter = *ptr;
            for(int ctr=0; ctr < iter; ++ctr){
               (*ptr)++;
            } 
            gthread_exit(NULL);
}

int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{ 
  int s_count[8];
  int tid[8];
  int ctr;
  void *retval;
  
  for(ctr=0; ctr<4; ++ctr){
        s_count[ctr] = ctr + 1;
        if(gthread_create(&tid[ctr], thfunc, &s_count[ctr]) < 0){
             printf("gthread_create failed\n");
             exit(-1);
        }
	printf("Created thread:\n");
   }        
  printf("In parent before sleep\n");
  sleep(5);
  sleep(5);
  sleep(5);
  sleep(5);
  printf("In parent after sleep\n");
  for(ctr=0; ctr<4; ++ctr){
        if(gthread_join(tid[ctr]))
             printf("Error\n");
        printf("Thread returned is %d\n", s_count[ctr]);
   }
  return 0;
}
