#include<ulib.h>
#include<gthread.h>

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
      printf("Created thread:\n");
  }        
  for(ctr=0; ctr < count; ++ctr){
        retval = gthread_join(tid[ctr]);
        printf("Thread returned is %d\n", *((int *)retval));
  }
}

struct tharg{
                char *ptr;
                int len;
                char pattern;
};

/*Thread functions must be declared as static*/
static void *thfunc(void *arg){
            struct tharg *ta = (struct tharg *)arg;
            int len = ta->len;
            for(int ctr=0; ctr < ta->len; ++ctr){
               *(ta->ptr + ctr) = ta->pattern;
            } 
            *(ta->ptr + ta->len) = 0;
            ta->len = 0;
            sleep(len);
            return ta;
}

int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{ 
  char buf[1024];
  int tid[8];
  struct tharg tas[8];
  int ctr;
  void *retval;
  
  for(ctr=0; ctr<4; ++ctr){
        tas[ctr].len = 4 + ctr * 4;
        tas[ctr].pattern = 'a' + ctr;
        tas[ctr].ptr = buf + ctr * 64;
        if(gthread_create(&tid[ctr], thfunc, &tas[ctr]) < 0){
             printf("gthread_create failed\n");
             exit(-1);
        }
	printf("Created thread:\n");
   }        
  for(ctr=3; ctr>=0; ctr--){
        struct tharg *ta = (struct tharg *)gthread_join(tid[ctr]);
        if(!ta){
                  printf("Error\n");
                  continue;
        }
        printf("Thread remains %d filled %s\n", ta->len, ta->ptr);
   }
  create_threads(4);
  return 0;
}
