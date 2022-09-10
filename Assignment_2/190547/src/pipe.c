#include<pipe.h>
#include<context.h>
#include<memory.h>
#include<lib.h>
#include<entry.h>
#include<file.h>


// Per process info for the pipe.
struct pipe_info_per_process {

    // TODO:: Add members as per your need...

    u32 pid;          // Stores the pid of the process whose information is stored
    int read_open;    // Stores whether the read port is open for the process
    int write_open;   // Stores whether the write port is open for the process

};

// Global information for the pipe.
struct pipe_info_global {

    char *pipe_buff;    // Pipe buffer: DO NOT MODIFY THIS.

    // TODO:: Add members as per your need... 
    
    int buff_len;      // Length of the string stored in the pipe
    int read_pos;      // Position to be read from 
    int write_pos;     // Position to write to

};

// Pipe information structure.
// NOTE: DO NOT MODIFY THIS STRUCTURE.
struct pipe_info {

    struct pipe_info_per_process pipe_per_proc [MAX_PIPE_PROC];
    struct pipe_info_global pipe_global;

};


// Function to allocate space for the pipe and initialize its members.
struct pipe_info* alloc_pipe_info () {
	
    // Allocate space for pipe structure and pipe buffer.
    struct pipe_info *pipe = (struct pipe_info*)os_page_alloc(OS_DS_REG);
    char* buffer = (char*) os_page_alloc(OS_DS_REG);

    // Return a NULL pointer if any of the above os_page_alloc fails
    if (pipe == NULL || buffer == NULL){
        return NULL;
    }

    // Assign pipe buffer.
    pipe->pipe_global.pipe_buff = buffer;

    /**
     *  TODO:: Initializing pipe fields
     *  
     *  Initialize per process fields for this pipe.
     *  Initialize global fields for this pipe.
     *
     */

    pipe->pipe_global.buff_len = 0;
    pipe->pipe_global.read_pos = 0;
    pipe->pipe_global.write_pos = 0;

    for (int i = 0; i < MAX_PIPE_PROC; i++){
        pipe->pipe_per_proc[i].read_open = 0;  // Initially all processes have read and write ends closed
        pipe->pipe_per_proc[i].write_open = 0;
        pipe->pipe_per_proc[i].pid = -1;       // pid = -1 means this index hasn't been assigned to any process
    }


    // Return the pipe.
    return pipe;

}

// Function to free pipe buffer and pipe info object.
// NOTE: DO NOT MODIFY THIS FUNCTION.
void free_pipe (struct file *filep) {

    os_page_free(OS_DS_REG, filep->pipe->pipe_global.pipe_buff);
    os_page_free(OS_DS_REG, filep->pipe);

}

// Fork handler for the pipe.
int do_pipe_fork (struct exec_context *child, struct file *filep) {

    /**
     *  TODO:: Implementation for fork handler
     *
     *  You may need to update some per process or global info for the pipe.
     *  This handler will be called twice since pipe has 2 file objects.
     *  Also consider the limit on no of processes a pipe can have.
     *  Return 0 on success.
     *  Incase of any error return -EOTHERS.
     *
     */


    // First look for the index which has already been assigned to this process process 
    // This happens when this function is called for the second file object
    int empty_index = 0;
    for (empty_index = 0; empty_index < MAX_PIPE_PROC; empty_index++){
        if (filep->pipe->pipe_per_proc[empty_index].pid == child->pid){
            break;
        }
    }

    // If no index has been assigned, then check if there is any empty index (pid == -1) available
    // i.e. the limit on no. of process has not been reached
    if (empty_index == MAX_PIPE_PROC){
        for (empty_index = 0; empty_index < MAX_PIPE_PROC; empty_index++){
            if (filep->pipe->pipe_per_proc[empty_index].pid == -1){
                break;
            }
        }
    }

    // If both the above cases are invalid, then return
    if (empty_index == MAX_PIPE_PROC){
        return -EOTHERS;
    }

    // Find the index which represents the parent process for this child
    u32 ppid = child->ppid;
    int parent;
    for (parent = 0; parent < MAX_PIPE_PROC; parent++){
        if (filep->pipe->pipe_per_proc[parent].pid == ppid){
            break;
         }
    }

    if (parent == MAX_PIPE_PROC){
        return -EOTHERS;
    }

    // Assign the per process pipe fields
    // For the function call for second file object of any pipe
    // the values are just overwritten with the same values
    filep->pipe->pipe_per_proc[empty_index].pid = child->pid;
    filep->pipe->pipe_per_proc[empty_index].read_open = filep->pipe->pipe_per_proc[parent].read_open;
    filep->pipe->pipe_per_proc[empty_index].write_open = filep->pipe->pipe_per_proc[parent].write_open;

    // Return successfully.
    return 0;

}

// Function to close the pipe ends and free the pipe when necessary.
long pipe_close (struct file *filep) {

    /**
     *  TODO:: Implementation of Pipe Close
     *
     *  Close the read or write end of the pipe depending upon the file
     *      object's mode.
     *  You may need to update some per process or global info for the pipe.
     *  Use free_pipe() function to free pipe buffer and pipe object,
     *      whenever applicable.
     *  After successful close, it return 0.
     *  Incase of any error return -EOTHERS.
     *
     */

    int ret_value;

    // Find the index of the process whose pipe is to be closed. 
    // Compare the pid's of all the processes in which pipe is valid with the current process pid
    struct exec_context *current = get_current_ctx();
    u32 pid = current->pid;
    int process_index;
    for (process_index = 0; process_index < MAX_PIPE_PROC; process_index++){
        if (filep->pipe->pipe_per_proc[process_index].pid == pid){
            break;
        }
    }

    // Return if no such index found
    if (process_index == MAX_PIPE_PROC){
        return -EOTHERS;
    }

    // If the end of the pipe to be closed is already closed, then the close function is not called (It returns -1 from entry.c itself I guess)

    // If the file to be closed is in read mode, make the read_open for the process as 0 in the pipe. 
    if (filep->mode == O_READ){
        filep->pipe->pipe_per_proc[process_index].read_open = 0;
    }

    // If the file to be closed is in write mode, make the write_open for the process as 0 in the pipe.
    if (filep->mode == O_WRITE){
        filep->pipe->pipe_per_proc[process_index].write_open = 0;
    }

    // If for this process both the ends have been closed, then make pid for that index as -1
    // This mean that this process has closed the pipe completely, and so the pipe doesn't need to store information about this process anymore. 
    if (filep->pipe->pipe_per_proc[process_index].read_open == 0 && filep->pipe->pipe_per_proc[process_index].write_open == 0){
        filep->pipe->pipe_per_proc[process_index].pid = -1;
    }

    int all_closed = 1;

    // If all the processes in which this pipe is there have closed all their ends, i.e. all pids are now -1, then remove this pipe.
    for (int i = 0; i < MAX_PIPE_PROC; i++){
        if (filep->pipe->pipe_per_proc[i].pid != -1) {
            all_closed = 0;
        }
    }

    if (all_closed){
        free_pipe(filep);
    }

    // Close the file and return.
    ret_value = file_close (filep);         // DO NOT MODIFY THIS LINE.

    // And return.
    return ret_value;

}

// Check whether passed buffer is valid memory location for read or write.
int is_valid_mem_range (unsigned long buff, u32 count, int access_bit) {

    /**
     *  TODO:: Implementation for buffer memory range checking
     *
     *  Check whether passed memory range is suitable for read or write.
     *  If access_bit == 1, then it is asking to check read permission.
     *  If access_bit == 2, then it is asking to check write permission.
     *  If range is valid then return 1.
     *  Incase range is not valid or have some permission issue return -EBADMEM.
     *
     */

    int ret_value = -EBADMEM;
    struct exec_context *ctx = get_current_ctx();

    int found_range = 0;   // Indicates whether the buffer is found in any of the segments

    for (int i = 0; i < MAX_MM_SEGS; i++){

        // For non stack segments, check only in the range of [start, next_free]
        if (i != MM_SEG_STACK){
            if (buff >= ctx->mms[i].start && buff+count-1 <= ctx->mms[i].next_free){
                found_range = 1;
                if ((ctx->mms[i].access_flags & access_bit) == access_bit){
                    ret_value = 1; // Modify ret_value only if memory range is suitable for the operation to be performed
                }
                break; // Break once the segment is found
            }
        }
        // For stack, check in the range of [start, end] 
        else {
            if (buff >= ctx->mms[i].start && buff+count-1 <= ctx->mms[i].end){
                found_range = 1;
                if ((ctx->mms[i].access_flags & access_bit) == access_bit){
                    ret_value = 1;
                }
                break;
            }
        }
    }

    // If buffer is not in the mm_segment, then search in vm_area in the range [vm_start, vm_end]
    if (!found_range){
        struct vm_area *vm_area = ctx->vm_area;
        while (vm_area != NULL){
            if (buff >= vm_area->vm_start && buff+count-1 <= vm_area->vm_end){
                found_range = 1;
                if((vm_area->access_flags & access_bit) == access_bit){
                    ret_value = 1;
                }
                break;
            }
            
            vm_area = vm_area->vm_next;
        }
    }

    // Return the finding.
    return ret_value;

}

// Function to read given no of bytes from the pipe.
int pipe_read (struct file *filep, char *buff, u32 count) {

    /**
     *  TODO:: Implementation of Pipe Read
     *
     *  Read the data from pipe buffer and write to the provided buffer.
     *  If count is greater than the present data size in the pipe then just read
     *       that much data.
     *  Validate file object's access right.
     *  On successful read, return no of bytes read.
     *  Incase of Error return valid error code.
     *       -EACCES: In case access is not valid.
     *       -EINVAL: If read end is already closed.
     *       -EOTHERS: For any other errors.
     *
     */

    int bytes_read = 0;

    // Validate the file mode
    if (!(filep->mode & O_READ)){
        return -EACCES;
    }

    // Find the index which represents this process
    struct exec_context *current = get_current_ctx();
    u32 pid = current->pid;
    int process_index;
    for (process_index = 0; process_index < MAX_PIPE_PROC; process_index++){
        if (filep->pipe->pipe_per_proc[process_index].pid == pid){
            break;
        }
    }

    // If no such process matching current pid is found, return -EOTHERS
    if (process_index == MAX_PIPE_PROC){
        return -EOTHERS;
    }

    // Check if the read port is open
    if (!(filep->pipe->pipe_per_proc[process_index].read_open)){
        return -EINVAL;
    }

    // Validate the buffer
    // Note the access_bit is 2 here, as buffer has to be written with data
    int mem_valid = is_valid_mem_range((unsigned long)buff, count, 2);
    if (mem_valid == -EBADMEM){
        return -EOTHERS;
    }


    int read_pos = filep->pipe->pipe_global.read_pos;

    // If count is greater than the pipe buffer, then buff_len bytes will be read
    if (filep->pipe->pipe_global.buff_len >= count){
        bytes_read = count;
    }
    else{
        bytes_read = filep->pipe->pipe_global.buff_len;
    }
    // Actual reading operation
    for (int i = 0; i < bytes_read; i++){
        buff[i] = filep->pipe->pipe_global.pipe_buff[(read_pos+i)%(MAX_PIPE_SIZE)];
    }

    // Reduce the buffer length, as the read bytes won't be read again
    filep->pipe->pipe_global.buff_len -= bytes_read;
    // Update the read position
    filep->pipe->pipe_global.read_pos = (read_pos + bytes_read) % (MAX_PIPE_SIZE);

    // Return no of bytes read.
    return bytes_read;

}

// Function to write given no of bytes to the pipe.
int pipe_write (struct file *filep, char *buff, u32 count) {

    /**
     *  TODO:: Implementation of Pipe Write
     *
     *  Write the data from the provided buffer to the pipe buffer.
     *  If count is greater than available space in the pipe then just write data
     *       that fits in that space.
     *  Validate file object's access right.
     *  On successful write, return no of written bytes.
     *  Incase of Error return valid error code.
     *       -EACCES: In case access is not valid.
     *       -EINVAL: If write end is already closed.
     *       -EOTHERS: For any other errors.
     *
     */

    int bytes_written = 0;

    // Validate the file mode
    if (!(filep->mode & O_WRITE)){
        return -EACCES;
    }

    // Find the index which represents this process
    struct exec_context *current = get_current_ctx();
    u32 pid = current->pid;
    int process_index;
    for (process_index = 0; process_index < MAX_PIPE_PROC; process_index++){
        if (filep->pipe->pipe_per_proc[process_index].pid == pid){
            break;
        }
    }

    // If process_index becomes MAX_PIPE_PROC, then no such process exists and so return -EOTHERS
    if (process_index == MAX_PIPE_PROC){
        return -EOTHERS;
    }

    // Check whether write port is open
    if (!(filep->pipe->pipe_per_proc[process_index].write_open)){
        return -EINVAL;
    }

    // Validate the memory range for read operation
    int mem_valid = is_valid_mem_range((unsigned long)buff, count, 1);
    if (mem_valid == -EBADMEM){
        return -EOTHERS;
    }


    int write_pos = filep->pipe->pipe_global.write_pos;

    // The availale space inside the pipe buffer
    int space = MAX_PIPE_SIZE - filep->pipe->pipe_global.buff_len;

    // If sufficient space is not there, then bytes written is equal to the available space
    if (space >= count) {
        bytes_written = count;
    }
    else {
        bytes_written = space;
    }

    // Actual write operation
    for (int i = 0; i < bytes_written; i++){
        filep->pipe->pipe_global.pipe_buff[(write_pos+i)%(MAX_PIPE_SIZE)] = buff[i];
    }

    // Increase the buff_len
    filep->pipe->pipe_global.buff_len += bytes_written;
    // Update the write position
    filep->pipe->pipe_global.write_pos = (write_pos + bytes_written) % (MAX_PIPE_SIZE);
    

    // Return no of bytes written.
    return bytes_written;

}

// Function to create pipe.
int create_pipe (struct exec_context *current, int *fd) {

    /**
     *  TODO:: Implementation of Pipe Create
     *
     *  Find two free file descriptors.
     *  Create two file objects for both ends by invoking the alloc_file() function. 
     *  Create pipe_info object by invoking the alloc_pipe_info() function and
     *       fill per process and global info fields.
     *  Fill the fields for those file objects like type, fops, etc.
     *  Fill the valid file descriptor in *fd param.
     *  On success, return 0.
     *  Incase of Error return valid Error code.
     *       -ENOMEM: If memory is not enough.
     *       -EOTHERS: Some other errors.
     *
     */

    // Create 2 file objects for read and write
    
    struct file* pipe_0 = alloc_file();
    // If alloc_file fails
    if (pipe_0 == NULL){
        return -ENOMEM;
    }

    struct file* pipe_1 = alloc_file();
    // If alloc_pipe fails
    if (pipe_1 == NULL){
        return -ENOMEM;
    }

    // Create one pointer for pipe info
    struct pipe_info *pipe = alloc_pipe_info();
    // If alloc_pipe_info fails
    if (pipe == NULL){
        return -ENOMEM;
    }
     
    // Assign per_process fields for this pipe
    pipe->pipe_per_proc[0].pid = current->pid;
    pipe->pipe_per_proc[0].read_open = 1;
    pipe->pipe_per_proc[0].write_open = 1;

    // Assign these pipes to the read and write objects
    pipe_0->pipe = pipe;
    pipe_1->pipe = pipe;

    // Assign various fields for the 2 file objects
    pipe_0->type = PIPE;
    pipe_0->mode = O_READ;
    pipe_0->ref_count = 1;
    pipe_0->fops->read = pipe_read;
    pipe_0->fops->close = pipe_close;
    
    pipe_1->type = PIPE;
    pipe_1->mode = O_WRITE;
    pipe_1->ref_count = 1;
    pipe_1->fops->write = pipe_write;
    pipe_1->fops->close = pipe_close;

    int i;
    // Find the minimum file descriptor empty
    for(i = 0; i < MAX_OPEN_FILES; i++){
        if (current->files[i] == NULL){
            fd[0] = i;
            break;
        }
    }

    // If no empty file descriptor
    if (i == MAX_OPEN_FILES){
        return -EOTHERS;
    }

    current->files[fd[0]] = pipe_0;

    // Find the next minimum file descriptor (just need to start checking from fd[0])
    for (i = fd[0]; i < MAX_OPEN_FILES; i++){
        if (current->files[i] == NULL){
            fd[1] = i;
            break;
        }
    }

    if (i == MAX_OPEN_FILES){
        return -EOTHERS;
    }

    current->files[fd[1]] = pipe_1;

    // Simple return.
    return 0;
}
