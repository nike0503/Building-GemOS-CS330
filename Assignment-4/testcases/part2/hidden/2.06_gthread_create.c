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

int create_threads(int count)
{ 
  int s_count[8];
  int tid[8];
  int ctr;
  void *retval;
 
  for(ctr=0; ctr < count; ++ctr){
      s_count[ctr] = ctr + 1;
      if(gthread_create(&tid[ctr], thfunc, &s_count[ctr]) < 0){
             printf("gthread_create failed\n");
             exit(-1);
      }
      printf("Created thread:\n");
  }        
  for(ctr=0; ctr < count; ++ctr){
        retval = gthread_join(tid[ctr]);
        printf("Thread returned is %d\n", *((int *)retval));
  }
}

static void *thfunc2(void *arg){

		int *ptr = (int *) arg;
                sleep(5);
                ptr = NULL;
                *ptr = 5;
		return ptr;
}

static void *thfunc1(void *arg){
		int *ptr = (int *) arg;
                sleep(10);
                ptr = NULL;
                *ptr = 5;
		return ptr;
}

int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{ 
  int t1_count = 2, t2_count=3;
  int tid1, tid2, tid3;
  void *retval;
  
   if(gthread_create(&tid1, thfunc1, &t1_count) < 0){
             printf("gthread_create failed\n");
             exit(-1);
   }
   printf("the thread I created is\n");
	
   if(gthread_create(&tid2, thfunc2, &t2_count) < 0){
             printf("gthread_create failed\n");
             exit(-1);
   }
   printf("the thread I created is \n");

   retval = gthread_join(tid1);
   printf("The value ptr is %x\n", retval);
   retval = gthread_join(tid2);
   printf("The value ptr is %x\n", retval);
   
   //create four threads now!
   create_threads(4); 
    
   return 0;
}
