#include<ulib.h>
#include<gthread.h>
/*Thread functions must be declared as static*/
#define BAR_PATTERN_INIT 0x77770
#define BAR_PATTERN_ALL_ALLOC 0x3C
#define BAR_PATTERN_ALL_ACCESS 0x3E

void atomic_OR(u64 *ptr, u64 val)
{
   asm volatile("lock or %%rsi, (%%rdi);"
                 :::"memory"
                );
} 
void alloc_and_init(u64 *uptr, int offset, int *flag, char pattern)
{
  for(int ctr=0; ctr<4; ctr++){
       void *lptr = gmalloc(4096, flag[ctr]);
       char *ptr;
       if(!lptr){
                printf("galloc failed %d\n", getpid());
                return;
       }
       uptr[offset+ctr] = (u64) lptr; 
       ptr = lptr;
       for(int i=0; i<4096; i++, ptr++){
             *ptr = (char)(pattern + ctr);
       }
  }
  return;
}
void free_all(u64 *uptr, int offset)
{
  for(int ctr=0; ctr<4; ctr++){
       void *lptr = (void *)uptr[offset+ctr];
       gfree(lptr);
  } 
}

void access(u64 *uptr, int read, int write)
{
  int loops = 16;
  while(loops--){
  for(int ctr=0; ctr<16; ctr++){
       char *ptr = (char *)uptr[ctr];
       int flag = (read >> ctr) & 0x1;
       if(flag && (!(ptr[1000] >= 'a' && ptr[1000] <= 'p')))
                printf("Error... should stop\n"); 
       if((write >> ctr) & 0x1){
          ptr[1000] = 'a';
       } 
  }  
  }
}

static void *thfunc(void *arg){
       int upoffset = 0;
       u64 *uptr = (u64 *)arg;
       int pid = getpid();
       int flag[4] = {GALLOC_OWNONLY, GALLOC_OWNONLY, GALLOC_OTRDONLY, GALLOC_OTRDWR};
 
       while(uptr[31] != BAR_PATTERN_INIT);
       // All wait here
       switch(pid){
             case 2: 
                      upoffset = 0;
                      alloc_and_init(uptr, upoffset, &flag[0], 'a');
                      atomic_OR(uptr + 30, 1 << pid);         
                      break;  
             case 3:
                      upoffset = 4;
                      alloc_and_init(uptr, upoffset, &flag[0], 'e');
                      atomic_OR(uptr + 30, 1 << pid);         
                      break;  
             case 4:
                      upoffset = 8;
                      alloc_and_init(uptr, upoffset, &flag[0], 'i');
                      atomic_OR(uptr + 30, 1 << pid);         
                      break;  
             case 5:
                      upoffset = 12;
                      alloc_and_init(uptr, upoffset, &flag[0], 'm');
                      atomic_OR(uptr + 30, 1 << pid);         
                      break;  
             default:
                     printf("Error. should not be printed\n");
                     break;
       }
       while(uptr[30] != BAR_PATTERN_ALL_ALLOC);
       
       for(int ctr=0; ctr<32; ++ctr){
            switch(pid){
                          case 2:
                                    access(uptr, 0xCCCF, 0x888F);
                                    break;
                          case 3:
                                    access(uptr, 0xCCFC, 0x88F8);
                                    break;
                          case 4:
                                    access(uptr, 0xCFCC, 0x8F88);
                                    break;
                          case 5:
                                    access(uptr, 0xFCCC, 0xF888);
                                    break;
                          default:
                               printf("Error. should not be printed\n");
                               break;
              }
                         
       } 
       atomic_OR(uptr+29, 1 << pid);
       while(uptr[29] != BAR_PATTERN_ALL_ACCESS);
       
       switch(pid){
                    case 2:
                              access(uptr, 0xCCCF, 0xFFFF);
                              break;
                    case 3:
                             sleep(10);
                             access(uptr, 0xCCC0, 0xFFF0);
                             break; 
                    case 4:
                             sleep(20);
                             access(uptr, 0xFF00, 0xFF00);
                             break; 
                    case 5:
                             sleep(30);
                             access(uptr, 0xF000, 0xF000);
                             break; 
       }  
       
       free_all(uptr, upoffset);
       return NULL;
}

int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{ 
  unsigned long *ptr;
  int tid[4];
  u64 maps[32] = {0};
  int pid = getpid();
  if(gthread_create(&tid[0], thfunc, &maps[0]) < 0){
             printf("gthread_create failed\n");
             exit(-1);
  }
  printf("Created first thread\n");
  if(gthread_create(&tid[1], thfunc, &maps[0]) < 0){
             printf("gthread_create failed\n");
             exit(-1);
  }
  printf("Created second thread\n");
  if(gthread_create(&tid[2], thfunc, &maps[0]) < 0){
             printf("gthread_create failed\n");
             exit(-1);
  }
  printf("Created third thread\n");
  if(gthread_create(&tid[3], thfunc, &maps[0]) < 0){
             printf("gthread_create failed\n");
             exit(-1);
  }
  printf("Created fourth thread\n", tid[3]);
  maps[31] = BAR_PATTERN_INIT;
  while(maps[30] != BAR_PATTERN_ALL_ALLOC);
 
  access(&maps[0], 0xFFFF, 0xFFFF); 
  atomic_OR(&maps[29], 1 << pid);
  while(maps[29] != BAR_PATTERN_ALL_ACCESS);
  
  gthread_join(tid[0]);
  gthread_join(tid[1]);
  gthread_join(tid[2]);
  gthread_join(tid[3]);
  printf("Success for all\n");
  return 0;
}
