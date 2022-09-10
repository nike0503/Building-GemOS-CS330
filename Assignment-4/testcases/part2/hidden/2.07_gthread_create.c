#include<ulib.h>
#include<gthread.h>
/*Thread functions must be declared as static*/

static void *thfun(void *arg){
            int *ptr = (int *) arg;
            int iter = *ptr;
            for(int ctr=0; ctr < iter; ++ctr){
               (*ptr)++;
            } 
            return ptr;
}

int create_threads(int count)
{ 
  int s_count[8];
  int tid[8];
  int ctr;
  void *retval;
 
  for(ctr=0; ctr < count; ++ctr){
      s_count[ctr] = ctr + 1;
      if(gthread_create(&tid[ctr], thfun, &s_count[ctr]) < 0){
             printf("gthread_create failed\n");
             exit(-1);
      }
      printf("Created thread:\n", tid[ctr]);
  }        
  for(ctr=0; ctr < count; ++ctr){
        retval = gthread_join(tid[ctr]);
        printf("Thread returned is %d\n", *((int *)retval));
  }
}

static void *thfunc2(void *arg){

		unsigned long *ptr = (unsigned long *) arg;
                sleep(5);
                (*ptr)++;
		return(ptr);
}

static void *thfunc1(void *arg){
		unsigned long *ptr = (unsigned long *) arg;
                sleep(20);
                (*ptr)++;
		gthread_exit(ptr);
}

int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{ 
  int tid1, tid2;
  unsigned long t1_count = 0x123456789;
  unsigned long t2_count = 0x987654321;
  void *retval;
  
   if(gthread_create(&tid1, thfunc1, &t1_count) < 0){
             printf("gthread_create failed\n");
             exit(-1);
   }
   printf("the thread I created is \n");
	
   if(gthread_create(&tid2, thfunc2, &t2_count) < 0){
             printf("gthread_create failed\n");
             exit(-1);
   }
   printf("the thread I created is\n");
   
   sleep(10);
   create_threads(2);

   retval = gthread_join(tid1);
   printf("The value ptr is %x\n", *(unsigned long *)retval);
   
   create_threads(3);
   retval = gthread_join(tid2);
   printf("The value ptr is %x\n", *(unsigned long *)retval);
    
   return 0;
}
