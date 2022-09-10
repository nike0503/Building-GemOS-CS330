#include <gthread.h>
#include <ulib.h>

static struct process_thread_info tinfo __attribute__((section(".user_data"))) = {};
/*XXX 
      Do not modifiy anything above this line. The global variable tinfo maintains user
      level accounting of threads. Refer gthread.h for definition of related structs.
 */

void call_exit() {    
    void *ret;
    asm volatile(
            "mov %%rax, %0;"
            : "=r" (ret)
            :
            :
            );

    gthread_exit(ret);
}


/* Returns 0 on success and -1 on failure */
/* Here you can use helper system call "make_thread_ready" for your implementation */
int gthread_create(int *tid, void *(*fc)(void *), void *arg) {
        
	/* You need to fill in your implementation here*/
    int i;
    for (i = 0; i < MAX_THREADS; i++) {
        if (tinfo.threads[i].status != TH_STATUS_ALIVE) {
            break;
        }
    }
    if (i == MAX_THREADS) {
        return -1;
    }
    void *stackp;
    stackp = mmap(NULL, TH_STACK_SIZE, PROT_READ|PROT_WRITE, 0);
    if (stackp == NULL) {
        return -1;
    }
    int pid = clone(fc, ((u64)stackp) + TH_STACK_SIZE - 8, (int *)arg);
    if (pid < 0) return -1;
    tinfo.num_threads++;
    tinfo.threads[i].status = (u8)TH_STATUS_ALIVE;
    tinfo.threads[i].tid = (u8)i;
    tinfo.threads[i].pid = (u8)pid;
    tinfo.threads[i].stack_addr = stackp;
    make_thread_ready(pid);
    *((u64 *)((u64)stackp + TH_STACK_SIZE - 8)) = (u64)call_exit; 
    *tid = i;
	return 0;
}

int gthread_exit(void *retval) {

	/* You need to fill in your implementation here*/
	
	int pid = getpid();

    for (int i = 0; i < MAX_THREADS; i++) {
        if (tinfo.threads[i].pid == pid && tinfo.threads[i].status == TH_STATUS_ALIVE) {
            tinfo.threads[i].ret_addr = retval;
        }
    }
	//call exit
	exit(0);
}

void* gthread_join(int tid) {
        
     /* Here you can use helper system call "wait_for_thread" for your implementation */
       
     /* You need to fill in your implementation here*/
    int id=-1;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (tinfo.threads[i].tid == tid) {
            id= i;

            break;
        }
    }
    if(id == -1) {return NULL;} 
    while(1) {
        int ret = wait_for_thread(tinfo.threads[id].pid);
        if(ret) break;
    }

    tinfo.num_threads--;
    tinfo.threads[id].status = (u8)TH_STATUS_USED;
    void *retval;
    retval = tinfo.threads[id].ret_addr;

    tinfo.threads[id].ret_addr = NULL;
	munmap(tinfo.threads[id].stack_addr, TH_STACK_SIZE);
    tinfo.threads[id].stack_addr = NULL;
    for (int i = 0; i < MAX_GALLOC_AREAS; i++) {
        if (tinfo.threads[id].priv_areas[i].owner != NULL) {
            munmap((void *)tinfo.threads[id].priv_areas[i].start, tinfo.threads[tid].priv_areas[i].length);

            tinfo.threads[id].priv_areas[i].start = 0;
            tinfo.threads[id].priv_areas[i].length = 0;
            tinfo.threads[id].priv_areas[i].flags = 0;
            tinfo.threads[id].priv_areas[i].owner = NULL;
        }
    }
    return retval;
}


/*Only threads will invoke this. No need to check if its a process
 * The allocation size is always < GALLOC_MAX and flags can be one
 * of the alloc flags (GALLOC_*) defined in gthread.h. Need to 
 * invoke mmap using the proper protection flags (for prot param to mmap)
 * and MAP_TH_PRIVATE as the flag param of mmap. The mmap call will be 
 * handled by handle_thread_private_map in the OS.
 * */

void* gmalloc(u32 size, u8 alloc_flag)
{
   
	/* You need to fill in your implementation here*/
    int flag;
    if (alloc_flag == GALLOC_OWNONLY) {
        flag = TP_SIBLINGS_NOACCESS;
    }
    else if (alloc_flag == GALLOC_OTRDONLY) {
        flag = TP_SIBLINGS_RDONLY;
    }
    else if (alloc_flag == GALLOC_OTRDWR) {
        flag = TP_SIBLINGS_RDWR;
    }
    else {
        return NULL;
    }
    int pid = getpid();
    int i;
    for (i = 0; i < MAX_THREADS; i++) {
        if (tinfo.threads[i].pid == pid && tinfo.threads[i].status == TH_STATUS_ALIVE) {
            break;
        }
    }
    if (i == MAX_THREADS) {
        return NULL;
    }
    int j;
    for (j = 0; j < MAX_GALLOC_AREAS; j++) {
        if (tinfo.threads[i].priv_areas[j].owner == NULL) {
            break;
        }
    }
    if (j == MAX_GALLOC_AREAS) {
        return NULL;
    }

    void *start = mmap(NULL, size, PROT_READ|PROT_WRITE|flag, MAP_TH_PRIVATE);
    if (start == NULL) {
        return NULL;
    }
    tinfo.threads[i].priv_areas[j].start = (u64)start;
    tinfo.threads[i].priv_areas[j].owner = &tinfo.threads[i];
    tinfo.threads[i].priv_areas[j].length = size;
    tinfo.threads[i].priv_areas[j].flags = (u32)alloc_flag;
    return start;
}
/*
   Only threads will invoke this. No need to check if the caller is a process.
*/
int gfree(void *ptr)
{
   
    /* You need to fill in your implementation here*/
    int pid = getpid();
    int i, j;
    for (i = 0; i < MAX_THREADS; i++) {
        for (j = 0; j < MAX_GALLOC_AREAS; j++) {
            if (tinfo.threads[i].priv_areas[j].start == (u64)ptr) {
                break;
            }
        }
        if (j != MAX_GALLOC_AREAS) {
            break;
        }
    }
    if (i == MAX_THREADS || j == MAX_GALLOC_AREAS) {
        return -1;
    }
 
    if (tinfo.threads[i].priv_areas[j].owner->pid != pid) {
        return -1;
    }
    if (munmap(ptr, tinfo.threads[i].priv_areas[j].length) < 0) {
        return -1;
    }

   
    tinfo.threads[i].priv_areas[j].owner = NULL;
    tinfo.threads[i].priv_areas[j].start = 0;
    tinfo.threads[i].priv_areas[j].length = 0;
    tinfo.threads[i].priv_areas[j].flags = 0;
    return 0;
}
