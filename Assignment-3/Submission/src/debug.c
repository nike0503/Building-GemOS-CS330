#include <debug.h>
#include <context.h>
#include <entry.h>
#include <lib.h>
#include <memory.h>


/*****************************HELPERS******************************************/

/*
 * allocate the struct which contains information about debugger
 *
 */
struct debug_info *alloc_debug_info()
{
	struct debug_info *info = (struct debug_info *) os_alloc(sizeof(struct debug_info));
	if(info)
		bzero((char *)info, sizeof(struct debug_info));
	return info;
}
/*
 * frees a debug_info struct
 */
void free_debug_info(struct debug_info *ptr)
{
	if(ptr)
		os_free((void *)ptr, sizeof(struct debug_info));
}



/*
 * allocates a page to store registers structure
 */
struct registers *alloc_regs()
{
	struct registers *info = (struct registers*) os_alloc(sizeof(struct registers));
	if(info)
		bzero((char *)info, sizeof(struct registers));
	return info;
}

/*
 * frees an allocated registers struct
 */
void free_regs(struct registers *ptr)
{
	if(ptr)
		os_free((void *)ptr, sizeof(struct registers));
}

/*
 * allocate a node for breakpoint list
 * which contains information about breakpoint
 */
struct breakpoint_info *alloc_breakpoint_info()
{
	struct breakpoint_info *info = (struct breakpoint_info *)os_alloc(
		sizeof(struct breakpoint_info));
	if(info)
		bzero((char *)info, sizeof(struct breakpoint_info));
	return info;
}

/*
 * frees a node of breakpoint list
 */
void free_breakpoint_info(struct breakpoint_info *ptr)
{
	if(ptr)
		os_free((void *)ptr, sizeof(struct breakpoint_info));
}

/*
 * Fork handler.
 * The child context doesnt need the debug info
 * Set it to NULL
 * The child must go to sleep( ie move to WAIT state)
 * It will be made ready when the debugger calls wait
 */
void debugger_on_fork(struct exec_context *child_ctx)
{
	// printk("DEBUGGER FORK HANDLER CALLED\n");
	child_ctx->dbg = NULL;
	child_ctx->state = WAITING;
    struct exec_context *parent = get_ctx_by_pid(child_ctx->ppid);
    parent->dbg->pid = child_ctx->pid;
}


/******************************************************************************/


/* This is the int 0x3 handler
 * Hit from the childs context
 */

void fill_debugee_regs (struct user_regs *regs, struct user_regs user_regs, int rdx_change) {
    regs->entry_rip = user_regs.entry_rip;
    regs->entry_rsp = user_regs.entry_rsp;
    regs->rbp = user_regs.rbp;
    regs->rax = user_regs.rax;

    regs->rdi = user_regs.rdi;
    regs->rsi = user_regs.rsi;
    regs->rdx = user_regs.rdx;
    regs->rcx = user_regs.rcx;
    regs->r8 = user_regs.r8;
    regs->r9 = user_regs.r9;

    regs->rdx += rdx_change;
}

long int3_handler(struct exec_context *ctx)
{
	
	//Your code
    struct exec_context *parent = get_ctx_by_pid(ctx->ppid);
    if (parent == NULL) {
        return -1;
    }
    
    u64 current_addr = (ctx->regs.entry_rip)-1;
    int found = 0;
    struct breakpoint_info *bp = parent->dbg->head;
    while (bp != NULL) {
        if (bp->addr == current_addr || bp->addr == current_addr - 1) {
            found = 1;
            break;
        }
        bp = bp->next;
    }
    if (found == 1) {      // which means call is not from end_handler
        if (*((u8 *)(ctx->regs.entry_rip - 2)) == PUSHRBP_OPCODE) { //pass control to child 
            *((u8 *)(ctx->regs.entry_rip - 2)) = INT3_OPCODE;
            *((u8 *)(ctx->regs.entry_rip - 1)) = parent->dbg->nxt_instr;
            --ctx->regs.entry_rip;
            parent->state = WAITING;
            ctx->state = READY;
            schedule(ctx);
        }
        else { // to pass control to parent
            
            for (int i = 0; i < MAX_BACKTRACE; i++) {
                parent->dbg->backtrace[i] = 0;
            }
            parent->dbg->backtrace[0] = ctx->regs.entry_rip - 1;
            parent->dbg->backtrace[1] = *((u64 *)ctx->regs.entry_rsp);
            int i = 2;
            u64 rbp = ctx->regs.rbp;
            while (*(u64 *)(rbp + 8) != END_ADDR) {
                if (*(u64 *)(rbp + 8) == (u64)parent->dbg->end_handler) {
                    parent->dbg->backtrace[i++] = *(u64 *)(rbp + 16);
                }
                else {
                    parent->dbg->backtrace[i++] = *(u64 *)(rbp + 8);
                }
                rbp = *((u64 *)rbp);
            }
            
            if (i == 2) { // means that this function is called from main
                parent->dbg->stack_count = 1;
                parent->dbg->stack[0].start_addr = ctx->regs.entry_rip - 1;
                parent->dbg->stack[0].return_addr = *((u64 *)ctx->regs.entry_rsp);
            }
            else { // it has been called from some other function
                parent->dbg->stack[parent->dbg->stack_count].start_addr = ctx->regs.entry_rip-1;
                parent->dbg->stack[parent->dbg->stack_count].return_addr = *((u64 *)ctx->regs.entry_rsp);
                parent->dbg->stack_count++; 
            }
            parent->dbg->nxt_instr = *((u8 *)ctx->regs.entry_rip);
            *((u8 *)ctx->regs.entry_rip) = INT3_OPCODE;
            parent->regs.rax = --ctx->regs.entry_rip;
            *((u8 *)ctx->regs.entry_rip) = PUSHRBP_OPCODE; 
            fill_debugee_regs(&parent->dbg->debugee_regs, ctx->regs, parent->dbg->rdx_change);
            if (bp->end_breakpoint_enable == 1) {
                ctx->regs.entry_rsp-=8;  
                parent->dbg->rdx_change += 8;
                *((u64 *)(ctx->regs.entry_rsp)) = (u64)parent->dbg->end_handler;   
            }
            
            ctx->state = WAITING;
            parent->state = READY;          
            schedule(parent);
        }
    }
    else {    // int3 is called from end_handler
        if (*((u8 *)(ctx->regs.entry_rip - 2)) == PUSHRBP_OPCODE) {
         
            *((u8 *)ctx->regs.entry_rip - 2) = INT3_OPCODE;
            *((u8 *)ctx->regs.entry_rip - 1) = parent->dbg->nxt_instr;
            --ctx->regs.entry_rip;
            parent->state = WAITING;
            ctx->state = READY;
            schedule(ctx);
        }
        else {
            for (int i = 0; i < MAX_BACKTRACE; i++) {
                parent->dbg->backtrace[i] = 0;
            }
            parent->dbg->backtrace[0] = *((u64 *)ctx->regs.entry_rsp);

            int i = 1;
            u64 rbp = ctx->regs.rbp;
            while (*(u64 *)(rbp + 8) != END_ADDR) {
                if (*(u64 *)(rbp + 8) == (u64)parent->dbg->end_handler) {
                    parent->dbg->backtrace[i++] = *(u64 *)(rbp + 16);
                }
                else {
                    parent->dbg->backtrace[i++] = *(u64 *)(rbp + 8);
                }

                rbp = *((u64 *)rbp);
            }   
            for (i = parent->dbg->stack_count-1; i >= 0; i--) {
                if (parent->dbg->stack[i].return_addr == *((u64 *)ctx->regs.entry_rsp)) {
                    break;
                }
            }
            parent->dbg->stack_count = i; 
            
            parent->dbg->nxt_instr = *((u8 *)ctx->regs.entry_rip);
            *((u8 *)ctx->regs.entry_rip) = INT3_OPCODE;
            parent->regs.rax = --ctx->regs.entry_rip;
            *((u8 *)ctx->regs.entry_rip) = PUSHRBP_OPCODE;
            fill_debugee_regs(&parent->dbg->debugee_regs, ctx->regs, parent->dbg->rdx_change);
            parent->dbg->rdx_change -= 8;
            ctx->state = WAITING;
            parent->state = READY;
            schedule(parent);
        }           
    }
	return 0;
}

/*
 * Exit handler.
 * Deallocate the debug_info struct if its a debugger.
 * Wake up the debugger if its a child
 */
void debugger_on_exit(struct exec_context *ctx)
{
	// Your code
    if (ctx->dbg == NULL) { // debugee process
        struct exec_context *parent = get_ctx_by_pid(ctx->ppid);
        parent->state = READY;
        parent->regs.rax = CHILD_EXIT;
    }
    else { // debugger
        struct breakpoint_info *bp = ctx->dbg->head;
        struct breakpoint_info *to_free;
        while (bp != NULL) {
            to_free = bp;
            bp = bp->next;
            free_breakpoint_info(to_free);
        }
        free_debug_info(ctx->dbg);
    }
}


/*
 * called from debuggers context
 * initializes debugger state
 */
int do_become_debugger(struct exec_context *ctx, void *addr)
{
	// Your code
    ctx->dbg = alloc_debug_info();
    if (ctx->dbg == NULL) {
        return -1;
    }

    ctx->dbg->head = NULL;
    ctx->dbg->breakpoint_count = 0;
    if (addr != NULL) {
        *((u8 *)addr) = INT3_OPCODE;
    }
    ctx->dbg->end_handler = addr;
    ctx->dbg->last_num = 0;
    ctx->dbg->stack_count = 0;
    ctx->dbg->rdx_change = 0;
    for (int i = 0; i < MAX_BACKTRACE; i++) {
        ctx->dbg->backtrace[i] = 0;
    }
	return 0;
}

/*
 * called from debuggers context
 */
int do_set_breakpoint(struct exec_context *ctx, void *addr, int flag)
{

	// Your code
    struct breakpoint_info *head = ctx->dbg->head;
    *((u8 *)addr) = INT3_OPCODE;  // Assumed that this instruction will be PUSHRBP
    if (head == NULL) {
        head = alloc_breakpoint_info();
        if (head == NULL) {
            return -1;
        }
        head->addr = (u64)addr;
        head->next = NULL;
        head->num = ++ctx->dbg->last_num;
        ctx->dbg->breakpoint_count++;
        head->end_breakpoint_enable = flag;
        ctx->dbg->head = head;
    }
    else {
        int on_stack = 0;
        while (head->next != NULL) {
            if (head->addr == (u64)addr) {
                for (int i = 0; i < ctx->dbg->stack_count; i++) {
                    if (ctx->dbg->stack[i].start_addr == (u64)addr) {
                        on_stack = 1;
                    }
                }
                if (on_stack == 1 && head->end_breakpoint_enable == 1 && flag == 0) {
                    return -1;
                }
                else if (on_stack == 1 && head->end_breakpoint_enable == 0 && flag == 1) {
                    *((u8 *)addr) = PUSHRBP_OPCODE;
                    head->end_breakpoint_enable = flag;
                    return 0;
                }
                else {
                    head->end_breakpoint_enable = flag;
                    return 0;
                }
            }
            head = head->next;
        }
        if (head->addr == (u64)addr) {
            for (int i = 0; i < ctx->dbg->stack_count; i++) {
                if (ctx->dbg->stack[i].start_addr == (u64)addr) {
                    on_stack = 1;
                }
            }
            if (on_stack == 1 && head->end_breakpoint_enable == 1 && flag == 0) {
                 return -1;
            }
            else if (on_stack == 1 && head->end_breakpoint_enable == 0 && flag == 1) {
                *((u8 *)addr) = PUSHRBP_OPCODE;
                head->end_breakpoint_enable = flag;
                return 0;
            }

            else {
                head->end_breakpoint_enable = flag;
                return 0;
            }
            return 0;
        }

        if (ctx->dbg->breakpoint_count == MAX_BREAKPOINTS) {
            return -1;
        }

        struct breakpoint_info *new = alloc_breakpoint_info();
        if (new == NULL) {
            return -1;
        }
        new->addr = (u64)addr;
        new->next = NULL;
        new->num = ++ctx->dbg->last_num; 
        ctx->dbg->breakpoint_count++;
        new->end_breakpoint_enable = flag;
        head->next = new;
    }
	return 0;
}

/*
 * called from debuggers context
 */
int do_remove_breakpoint(struct exec_context *ctx, void *addr)
{
	//Your code
    struct breakpoint_info *bp = ctx->dbg->head;
    int on_stack = 0;
    for (int i = 0; i < ctx->dbg->stack_count; i++) {
        if (ctx->dbg->stack[i].start_addr == (u64)addr) {
            on_stack = 1;
        }
    }

    if (bp->addr == (u64)addr) {
        if (bp->end_breakpoint_enable == 1 && on_stack == 1) {
            return -1;
        }
        ctx->dbg->head = bp->next;
        *((u8 *)addr) = PUSHRBP_OPCODE;
        free_breakpoint_info(bp);
    }
    else {
        while (bp->next != NULL && bp->next->addr != (u64)addr) {
            bp = bp->next;
        }
        if (bp->next == NULL) {
            return -1;
        }
        if (bp->end_breakpoint_enable == 1 && on_stack == 1) {
            return -1;
        }

        struct breakpoint_info *to_remove = bp->next;
        bp->next = bp->next->next;
        free_breakpoint_info(to_remove);
        *((u8 *)addr) = PUSHRBP_OPCODE;
    }
    ctx->dbg->breakpoint_count--;
	return 0;
}


/*
 * called from debuggers context
 */

int do_info_breakpoints(struct exec_context *ctx, struct breakpoint *ubp)
{
	
	// Your code
    struct breakpoint_info *bp = ctx->dbg->head;
    int i = 0;
    while (bp != NULL) { 
        ubp[i].addr = bp->addr;
        ubp[i].num = bp->num;
        ubp[i].end_breakpoint_enable = bp->end_breakpoint_enable;
        i++;
        bp = bp->next;
    }
	return i;
}


/*
 * called from debuggers context
 */
int do_info_registers(struct exec_context *ctx, struct registers *regs)
{
	// Your code
    regs->entry_rip = ctx->dbg->debugee_regs.entry_rip;
    regs->entry_rsp = ctx->dbg->debugee_regs.entry_rsp;
    regs->rbp = ctx->dbg->debugee_regs.rbp;
    regs->rax = ctx->dbg->debugee_regs.rax;

    regs->rdi = ctx->dbg->debugee_regs.rdi;
    regs->rsi = ctx->dbg->debugee_regs.rsi;
    regs->rdx = ctx->dbg->debugee_regs.rdx;
    regs->rcx = ctx->dbg->debugee_regs.rcx;
    regs->r8 = ctx->dbg->debugee_regs.r8;
    regs->r9 = ctx->dbg->debugee_regs.r9;
    return 0;
}

/*
 * Called from debuggers context
 */
int do_backtrace(struct exec_context *ctx, u64 bt_buf)
{

	// Your code
    int i = 0;
    u64 *bt = (u64 *)bt_buf;
    while (ctx->dbg->backtrace[i] != 0) {
        bt[i] = ctx->dbg->backtrace[i];
        i++;
    }

    return i;
}

/*
 * When the debugger calls wait
 * it must move to WAITING state
 * and its child must move to READY state
 */

s64 do_wait_and_continue(struct exec_context *ctx)
{
	// Your code
    int debugee_pid = ctx->dbg->pid;
    struct exec_context *next_ctx = get_ctx_by_pid(debugee_pid);
    ctx->state = WAITING;
    next_ctx->state = READY;
    schedule(next_ctx);
    return -1;
}






