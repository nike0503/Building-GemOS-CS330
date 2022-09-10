#include<clone_threads.h>
#include<entry.h>
#include<context.h>
#include<memory.h>
#include<lib.h>
#include<mmap.h>

void do_page_walk (u32 pgd, struct thread_private_map map, int cur) {

    int no_iter = map.length/PAGE_SIZE;
    for (int i = 0; i < no_iter; i++) {
        u64 addr = map.start_addr +  i*PAGE_SIZE;
    u32 offsetL1 = 0x1FF & (addr >> PGD_SHIFT);
    u32 offsetL2 = 0x1FF & (addr >> PUD_SHIFT);
    u32 offsetL3 = 0x1FF & (addr >> PMD_SHIFT);
    u32 offsetL4 = 0x1FF & (addr >> PTE_SHIFT);
  
    u64 *l1 = osmap(pgd);
    u64 *l2, *l3, *l4;
    u32 pfn2, pfn3, pfn4, dataPfn;

    if (*(l1 + offsetL1) & 1) {
        pfn2 = *(l1 + offsetL1) >> PTE_SHIFT;
    }
    else {
        return;
    }

    l2 = osmap(pfn2);
    if (*(l2 + offsetL2) & 1) {
        pfn3 = *(l2 + offsetL2) >> PTE_SHIFT;
    }
    else {
        return;        
    }

    l3 = osmap(pfn3);
    if (*(l3 + offsetL3) & 1) {
        pfn4 = *(l3 + offsetL3) >> PTE_SHIFT;
    }
    else {
        return;
    }

    //mapping to Physical page
    l4 = osmap(pfn4);
    if (*(l4 + offsetL4) & 1) {
        dataPfn = *(l4 + offsetL4) >> PTE_SHIFT;
        int access = map.flags & 0x70;
        if (access == TP_SIBLINGS_NOACCESS && cur == 1) {
        
            *(l4 + offsetL4) = (dataPfn << PTE_SHIFT) | (0x1);
        }
        else if (access == TP_SIBLINGS_RDONLY && cur == 1) {
            *(l4 + offsetL4) = (dataPfn << PTE_SHIFT) | (0x5);
        }
        else {
  
            *(l4 + offsetL4) = (dataPfn << PTE_SHIFT) | (0x7);
        }
    }
    else {
        return;
    }
    asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
    }

}

/*
  system call handler for clone, create thread like 
  execution contexts. Returns pid of the new context to the caller. 
  The new context starts execution from the 'th_func' and 
  use 'user_stack' for its stack
*/
long do_clone(void *th_func, void *user_stack, void *user_arg) 
{
  


  struct exec_context *new_ctx = get_new_ctx();
  struct exec_context *ctx = get_current_ctx();

  u32 pid = new_ctx->pid;
  
  if(!ctx->ctx_threads){  // This is the first thread
          ctx->ctx_threads = os_alloc(sizeof(struct ctx_thread_info));
          bzero((char *)ctx->ctx_threads, sizeof(struct ctx_thread_info));
          ctx->ctx_threads->pid = ctx->pid;
  }
     
 /* XXX Do not change anything above. Your implementation goes here*/
  
  
  // allocate page for os stack in kernel part of process's VAS
  // The following two lines should be there. The order can be 
  // decided depending on your logic.

//  setup_child_context(new_ctx);
//  new_ctx->type = EXEC_CTX_USER_TH;    // Make sure the context type is thread

  int i;
  for (i = 0; i < MAX_THREADS; i++){
    if (ctx->ctx_threads->threads[i].status == TH_UNUSED) {
        ctx->ctx_threads->threads[i].status = TH_USED;
        ctx->ctx_threads->threads[i].pid = pid;
        ctx->ctx_threads->threads[i].parent_ctx = ctx;
        break;
    }
  }
  if (i == MAX_THREADS) {
      return -1;
  }

  *new_ctx = *ctx;
  new_ctx->pid = pid;
  new_ctx->ppid = ctx->pid;
  new_ctx->regs.entry_rsp = (u64)user_stack;
  new_ctx->regs.entry_rip = (u64)th_func;
  new_ctx->regs.rdi = (u64)user_arg;
  setup_child_context(new_ctx);
  new_ctx->type = EXEC_CTX_USER_TH;    // Make sure the context type is thread
  
  new_ctx->state = READY;            // For the time being. Remove it as per your need.
  return pid;

}

/*This is the page fault handler for thread private memory area (allocated using 
 * gmalloc from user space). This should fix the fault as per the rules. If the the 
 * access is legal, the fault handler should fix it and return 1. Otherwise it should
 * invoke segfault_exit and return -1*/

int handle_thread_private_fault(struct exec_context *current, u64 addr, int error_code)
{
  
   /* your implementation goes here*/

    int i,j;
    struct thread *th;
    struct thread_private_map *map = NULL; 
    struct ctx_thread_info *tinfo = current->ctx_threads;
    for (i = 0; i < MAX_THREADS; i++) {
        struct thread *t = &tinfo->threads[i];
        if (t->status == TH_UNUSED) {
            break;
        }
        for (j = 0; j < MAX_PRIVATE_AREAS; j++) {
            struct thread_private_map *m = &t->private_mappings[j];
            if (m->owner && (addr >= m->start_addr) && (addr < m->start_addr + m->length)) {
                break;
            }
        }
        if (j != MAX_PRIVATE_AREAS) {
            break;
        }
    }
    if (i == MAX_THREADS || j == MAX_PRIVATE_AREAS) {
        return -1;
    }
    th = &tinfo->threads[i];
    map = &th->private_mappings[j];
    struct exec_context *th_ctx = get_ctx_by_pid(th->pid);
    if (th_ctx->pid == current->pid || th_ctx->ppid == current->pid) {
        // Current thread is same as owner
               
    }
    else {
        int access = map->flags & 0x70;
        
        if (access == TP_SIBLINGS_NOACCESS) {
            segfault_exit(current->pid, current->regs.entry_rip, addr);
            return -1;
        }
        if (access == TP_SIBLINGS_RDONLY && (error_code & (int)PF_ERROR_WR == (int)PF_ERROR_WR)) { 
            segfault_exit(current->pid, current->regs.entry_rip, addr);
            return -1;
        }
    }
   /* if (error_code & 0x2 == 0x2 && (map->flags & 0x70 != TP_SIBLINGS_RDWR)) {
   
        segfault_exit(current->pid, current->regs.entry_rip, addr);
        return -1;
    }
    if (error_code & 0x2 == 0 && (map->flags & 0x70 == TP_SIBLINGS_NOACCESS)) {
 
        segfault_exit(current->pid, current->regs.entry_rip, addr);
        return -1;
    }*/
    u32 offsetL1 = 0x1FF & (addr >> PGD_SHIFT);
    u32 offsetL2 = 0x1FF & (addr >> PUD_SHIFT);
    u32 offsetL3 = 0x1FF & (addr >> PMD_SHIFT);
    u32 offsetL4 = 0x1FF & (addr >> PTE_SHIFT);
    
    u64 *l1 = osmap(current->pgd);
    u64 *l2, *l3, *l4;
    u32 pfn2, pfn3, pfn4, dataPfn;

    if (*(l1 + offsetL1) & 1) {
        pfn2 = *(l1 + offsetL1) >> PTE_SHIFT;
    }
    else {
        pfn2 = os_pfn_alloc(OS_PT_REG);
        *(l1 + offsetL1) = (pfn2 << PTE_SHIFT) | (0x7);
    }

    l2 = osmap(pfn2);
    if (*(l2 + offsetL2) & 1) {
        pfn3 = *(l2 + offsetL2) >> PTE_SHIFT;
    }
    else {
        pfn3 = os_pfn_alloc(OS_PT_REG);
        *(l2 + offsetL2) = (pfn3 << PTE_SHIFT) | (0x7);
    }

    l3 = osmap(pfn3);
    if (*(l3 + offsetL3) & 1) {
        pfn4 = *(l3 + offsetL3) >> PTE_SHIFT;
    }
    else {
        pfn4 = os_pfn_alloc(OS_PT_REG);
        *(l3 + offsetL3) = (pfn4 << PTE_SHIFT) | (0x7);
    }

    //mapping to Physical page
    l4 = osmap(pfn4);
    if (*(l4 + offsetL4) & 1) {
        dataPfn = *(l4 + offsetL4) >> PTE_SHIFT;
    }
    else {
        int access = map->flags & 0x70;
        dataPfn = os_pfn_alloc(USER_REG);
        *(l4 + offsetL4) = (dataPfn << PTE_SHIFT) | (0x7);
    }
    return 1;
}

/*This is a handler called from scheduler. The 'current' refers to the outgoing context and the 'next' 
 * is the incoming context. Both of them can be either the parent process or one of the threads, but only
 * one of them can be the process (as we are having a system with a single user process). This handler
 * should apply the mapping rules passed in the gmalloc calls. */

int handle_private_ctxswitch(struct exec_context *current, struct exec_context *next)
{
  
    /* your implementation goes here*/

    struct thread *cth = NULL, *nth = NULL;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (current->ctx_threads->threads[i].pid == current->pid) {
            cth = &current->ctx_threads->threads[i];
        }
        if (current->ctx_threads->threads[i].pid == next->pid) {
            nth = &current->ctx_threads->threads[i];
        }
    }
    for (int i = 0; i < MAX_PRIVATE_AREAS; i++) {
        if (cth != NULL && nth != NULL) {
            if (cth->private_mappings[i].owner != NULL) {
                do_page_walk(current->pgd, cth->private_mappings[i], 1);
            }
            if (nth->private_mappings[i].owner != NULL) {
                do_page_walk(next->pgd, nth->private_mappings[i], 0);
            }
        }
        else if (cth == NULL) {
          for (int j = 0; j < MAX_THREADS; j++) {
                if (current->ctx_threads->threads[j].pid != nth->pid && current->ctx_threads->threads[j].status == TH_USED) {
                    if (current->ctx_threads->threads[j].private_mappings[i].owner != NULL)
                        do_page_walk(current->pgd, current->ctx_threads->threads[j].private_mappings[i], 1);
                }
            }
            if (nth->private_mappings[i].owner != NULL)
                do_page_walk(next->pgd, nth->private_mappings[i], 0);
        }
    }
    return 0;	

}
