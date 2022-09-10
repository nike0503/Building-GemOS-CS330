#include<ulib.h>
#include<gthread.h>
/*Thread functions must be declared as static*/

static void *thfunc2(void *arg){

		int *ptr = (int *) arg;
                *ptr = 5;
		return ptr;
}

static void *thfunc1(void *arg){
		int *ptr = (int *) arg;
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
   printf("the thread I created\n");
	
   if(gthread_create(&tid2, thfunc2, &t2_count) < 0){
             printf("gthread_create failed\n");
             exit(-1);
   }
   printf("the thread I created\n");

   retval = gthread_join(tid1);
   printf("The value ptr is %x\n", retval);
   retval = gthread_join(tid2);
   printf("The value returned is %d\n", *((int *)retval));

   return 0;
}
