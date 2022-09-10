#include<ppipe.h>
#include<context.h>
#include<memory.h>
#include<lib.h>
#include<entry.h>
#include<file.h>


// Per process information for the ppipe.
struct ppipe_info_per_process {

    // TODO:: Add members as per your need...
    
    u32 pid;           // Stores the pid of this process
    int read_open;     // Stores whether read port is open
    int write_open;    // Stores whether write port is open

    int read_pos;      // Position from which this process can read
    int bytes_read;    // Offset from the buffer start which has been read by this pipe, i.e. the process reads from buff_start + bytes_read

};

// Global information for the ppipe.
struct ppipe_info_global {

    char *ppipe_buff;       // Persistent pipe buffer: DO NOT MODIFY THIS.

    // TODO:: Add members as per your need...
    
    int buff_len;      // Length of the string stored in the ppipe_buffer

    int buff_start;    // Starting position of the string
    int buff_end;      // Ending position of the string

    // The positions from buff_start - buff_end (in cyclic order) cannot be written by any process.
    // buff_end is the position at which any process starts to write     
};

// Persistent pipe structure.
// NOTE: DO NOT MODIFY THIS STRUCTURE.
struct ppipe_info {

    struct ppipe_info_per_process ppipe_per_proc [MAX_PPIPE_PROC];
    struct ppipe_info_global ppipe_global;

};


// Function to allocate space for the ppipe and initialize its members.
struct ppipe_info* alloc_ppipe_info() {

    // Allocate space for ppipe structure and ppipe buffer.
    struct ppipe_info *ppipe = (struct ppipe_info*)os_page_alloc(OS_DS_REG);
    char* buffer = (char*) os_page_alloc(OS_DS_REG);

    if (ppipe == NULL || buffer ==NULL){
        return NULL;
    }

    // Assign ppipe buffer.
    ppipe->ppipe_global.ppipe_buff = buffer;

    /**
     *  TODO:: Initializing pipe fields
     *
     *  Initialize per process fields for this ppipe.
     *  Initialize global fields for this ppipe.
     *
     */ 

    // Initial values are similar to those in normal pipe
    ppipe->ppipe_global.buff_len = 0;
    ppipe->ppipe_global.buff_start = 0;
    ppipe->ppipe_global.buff_end = 0;

    for (int i = 0; i < MAX_PPIPE_PROC; i++){
        ppipe->ppipe_per_proc[i].pid = -1;
        ppipe->ppipe_per_proc[i].read_pos = 0;
        ppipe->ppipe_per_proc[i].bytes_read = 0;
        ppipe->ppipe_per_proc[i].read_open = 0;
        ppipe->ppipe_per_proc[i].write_open = 0;
    }



    // Return the ppipe.
    return ppipe;

}

// Function to free ppipe buffer and ppipe info object.
// NOTE: DO NOT MODIFY THIS FUNCTION.
void free_ppipe (struct file *filep) {

    os_page_free(OS_DS_REG, filep->ppipe->ppipe_global.ppipe_buff);
    os_page_free(OS_DS_REG, filep->ppipe);

} 

// Fork handler for ppipe.
int do_ppipe_fork (struct exec_context *child, struct file *filep) {
    
    /**
     *  TODO:: Implementation for fork handler
     *
     *  You may need to update some per process or global info for the ppipe.
     *  This handler will be called twice since ppipe has 2 file objects.
     *  Also consider the limit on no of processes a ppipe can have.
     *  Return 0 on success.
     *  Incase of any error return -EOTHERS.
     *
     */

    // The logic for fork is similar to that used in normal pipe
    // First look for the index which has been assigned to this child
    // If not found, look for an empty index
    // After finding the index, assign the values of per process fields as the same as that of its parent process
    int empty_index = 0;
    for (empty_index = 0; empty_index < MAX_PPIPE_PROC; empty_index++){
        if (filep->ppipe->ppipe_per_proc[empty_index].pid == child->pid){
            break;
        }
    }
    if (empty_index == MAX_PPIPE_PROC){
        for (empty_index = 0; empty_index < MAX_PPIPE_PROC; empty_index++){
            if (filep->ppipe->ppipe_per_proc[empty_index].pid == -1){
                break;
            }
        }
    }

    if (empty_index == MAX_PPIPE_PROC){
        return -EOTHERS;
    }
    
    u32 ppid = child->ppid;
    int parent;
    for (parent = 0; parent < MAX_PPIPE_PROC; parent++){
        if (filep->ppipe->ppipe_per_proc[parent].pid == ppid){
            break;
        }
    }
    if (parent == MAX_PPIPE_PROC){
        return -EOTHERS;
    }

    // If this function is called multiple times, then also it doesn't affect these assignments, as they remain the same for both the calls
    filep->ppipe->ppipe_per_proc[empty_index].pid = child->pid;
    filep->ppipe->ppipe_per_proc[empty_index].read_open = filep->ppipe->ppipe_per_proc[parent].read_open;
    filep->ppipe->ppipe_per_proc[empty_index].write_open = filep->ppipe->ppipe_per_proc[parent].write_open;
    filep->ppipe->ppipe_per_proc[empty_index].read_pos = filep->ppipe->ppipe_per_proc[parent].read_pos;
    filep->ppipe->ppipe_per_proc[empty_index].bytes_read = filep->ppipe->ppipe_per_proc[parent].bytes_read;


    // Return successfully.
    return 0;

}


// Function to close the ppipe ends and free the ppipe when necessary.
long ppipe_close (struct file *filep) {

    /**
     *  TODO:: Implementation of Pipe Close
     *
     *  Close the read or write end of the ppipe depending upon the file
     *      object's mode.
     *  You may need to update some per process or global info for the ppipe.
     *  Use free_pipe() function to free ppipe buffer and ppipe object,
     *      whenever applicable.
     *  After successful close, it return 0.
     *  Incase of any error return -EOTHERS.
     *                                                                          
     */

    // The logic for close is also the same as that used in normal pipe
    // Find the current process index in the ppipe_per_proc array
    // Close the appropriate port
    // If both the ports closed, then remove the process from the array
    // If all ports for all processes have been closed, delete the ppipe 
    int ret_value;

    struct exec_context *current = get_current_ctx();
    u32 pid = current->pid;
    int process_index;
    for (process_index = 0; process_index < MAX_PPIPE_PROC; process_index++){
        if (filep->ppipe->ppipe_per_proc[process_index].pid == pid){
            break;
        }
    }

    if (process_index == MAX_PPIPE_PROC){
        return -EOTHERS;
    }

    if (filep->mode == O_READ){
        filep->ppipe->ppipe_per_proc[process_index].read_open = 0;
    }

    if (filep->mode == O_WRITE){
        filep->ppipe->ppipe_per_proc[process_index].write_open = 0;
    }

    int all_closed = 1;

    if (filep->ppipe->ppipe_per_proc[process_index].read_open == 0 && filep->ppipe->ppipe_per_proc[process_index].write_open == 0){
        filep->ppipe->ppipe_per_proc[process_index].pid == -1;
    }

    for (int i = 0; i < MAX_PPIPE_PROC; i++){
        if (filep->ppipe->ppipe_per_proc[i].pid != -1) {
            all_closed = 0;
        }
    }

    if (all_closed){
        free_ppipe(filep);
    }
    // Close the file.
    ret_value = file_close (filep);         // DO NOT MODIFY THIS LINE.

    // And return.
    return ret_value;

}

// Function to perform flush operation on ppipe.
int do_flush_ppipe (struct file *filep) {

    /**
     *  TODO:: Implementation of Flush system call
     *
     *  Reclaim the region of the persistent pipe which has been read by 
     *      all the processes.
     *  Return no of reclaimed bytes.
     *  In case of any error return -EOTHERS.
     *
     */

    // For this function first find the number of bytes in the buffer which have been read by all the processes.
    // For any particular process, that is stored in the bytes_read field
    // Find the minimum of this over all processes
    // This would be the number of bytes to be reclaimed
    // Then simply reduce the buff_len by these many bytes, and increase the buff_start by those many bytes.
    // Also reduce the bytes_read in all the processes with read end opened by reclaimed bytes
    // If read ends of all processes are closed, reclaim 0 bytes
    int reclaimed_bytes = 0;

    int read_by_all = filep->ppipe->ppipe_global.buff_len; // Assuming all the bytes of the buffer have been read initially

    int buff_start = filep->ppipe->ppipe_global.buff_start;

    int all_closed = 1; // Whether the read end for all the processes is closed
    
    for (int i = 0; i < MAX_PPIPE_PROC; i++){
        if (!(filep->ppipe->ppipe_per_proc[i].read_open)){ // We need to check only for those processes whose read port is open
            continue;
        }
        all_closed = 0; // If the read end for any process is open, then make it 0

        int bytes_read = filep->ppipe->ppipe_per_proc[i].bytes_read;

        // If any process has read lesser bytes, then reclaimed bytes should be reduced
        if (bytes_read < read_by_all){
            read_by_all = bytes_read;
        }
    }

    // If read ends of all processes is not closed, only then update the reclaimed bytes, otherwise let it remain 0.
    if (all_closed == 0){
        reclaimed_bytes = read_by_all;
    }

    // Update the start of buffer to new value after reclaiming the bytes
    filep->ppipe->ppipe_global.buff_start = (buff_start + reclaimed_bytes) % (MAX_PPIPE_SIZE);

    // Reduce the buffer length
    filep->ppipe->ppipe_global.buff_len -= reclaimed_bytes;

    // For all processes with read ends open, reduce the bytes read
    for (int i = 0; i < MAX_PPIPE_PROC; i++){
        if (filep->ppipe->ppipe_per_proc[i].read_open){
            filep->ppipe->ppipe_per_proc[i].bytes_read -= reclaimed_bytes;
        }
    } 


    // Return reclaimed bytes.
    return reclaimed_bytes;

}

// Read handler for the ppipe.
int ppipe_read (struct file *filep, char *buff, u32 count) {
    
    /**
     *  TODO:: Implementation of PPipe Read
     *
     *  Read the data from ppipe buffer and write to the provided buffer.
     *  If count is greater than the present data size in the ppipe then just read
     *      that much data.
     *  Validate file object's access right.
     *  On successful read, return no of bytes read.
     *  Incase of Error return valid error code.
     *      -EACCES: In case access is not valid.
     *      -EINVAL: If read end is already closed.
     *      -EOTHERS: For any other errors.
     *
     */

    // First validate the file mode and read port
    int bytes_read = 0;
    if (!(filep->mode & O_READ)){
        return -EACCES;
    }

    struct exec_context *current = get_current_ctx();
    u32 pid = current->pid;
    int process_index;
    for (process_index = 0; process_index < MAX_PPIPE_PROC; process_index++){
        if (filep->ppipe->ppipe_per_proc[process_index].pid == pid){
            break;
        }
    }

    if (process_index == MAX_PPIPE_PROC){
        return -EOTHERS;
    }

    if (!(filep->ppipe->ppipe_per_proc[process_index].read_open)){
        return -EINVAL;
    }


    int read_pos = filep->ppipe->ppipe_per_proc[process_index].read_pos;
    
    // The number of bytes from the buffer that can be read (Total length - the length of bytes already read)
    int read_limit = filep->ppipe->ppipe_global.buff_len - filep->ppipe->ppipe_per_proc[process_index].bytes_read;

    // If this limit is greater than count, then read count number of bytes, otherwise read only this limit number of bytes
    if (read_limit >= count){
        bytes_read = count;
    }
    else{
        bytes_read = read_limit;
    }

    // Start reading from read_pos
    for (int i = 0; i < bytes_read; i++){
        buff[i] = filep->ppipe->ppipe_global.ppipe_buff[(read_pos+i)%(MAX_PPIPE_SIZE)];
    }

    // Update the read position in the end
    filep->ppipe->ppipe_per_proc[process_index].read_pos = (read_pos + bytes_read) % (MAX_PPIPE_SIZE);

    // Update the no of bytes read
    filep->ppipe->ppipe_per_proc[process_index].bytes_read += bytes_read;

    // Return no of bytes read.
    return bytes_read;
	
}

// Write handler for ppipe.
int ppipe_write (struct file *filep, char *buff, u32 count) {

    /**
     *  TODO:: Implementation of PPipe Write
     *
     *  Write the data from the provided buffer to the ppipe buffer.
     *  If count is greater than available space in the ppipe then just write
     *      data that fits in that space.
     *  Validate file object's access right.
     *  On successful write, return no of written bytes.
     *  Incase of Error return valid error code.
     *      -EACCES: In case access is not valid.
     *      -EINVAL: If write end is already closed.
     *      -EOTHERS: For any other errors.
     *
     */

    // First validate the file mode and the write port
    int bytes_written = 0;

    if (!(filep->mode & O_WRITE)){
        return -EACCES;
    }

    struct exec_context *context = get_current_ctx();
    u32 pid = context->pid;
    int process_index;
    for (process_index = 0; process_index < MAX_PPIPE_PROC; process_index++){
        if (filep->ppipe->ppipe_per_proc[process_index].pid == pid){
            break;
        }
    }

    if (process_index == MAX_PPIPE_PROC){
        return -EOTHERS;
    }

    if (!(filep->ppipe->ppipe_per_proc[process_index].write_open)){
        return -EINVAL;
    }

    // Write for ppipe is somewhat similar to that in pipe as all processes have a common position from which they start to write

    int write_pos = filep->ppipe->ppipe_global.buff_end; // We need to write from buffer end

    // Find the available space inside the buffer
    int space = MAX_PPIPE_SIZE - filep->ppipe->ppipe_global.buff_len;

    if (space >= count){
        bytes_written = count;
    }
    else{
        bytes_written = space;
    }

    // Start writing from write_pos, i.e. the buffer end
    for (int i = 0; i < bytes_written; i++){
        filep->ppipe->ppipe_global.ppipe_buff[(write_pos+i)%(MAX_PPIPE_SIZE)] = buff[i];
    }

    filep->ppipe->ppipe_global.buff_len += bytes_written;
    filep->ppipe->ppipe_global.buff_end = (write_pos + bytes_written) % (MAX_PPIPE_SIZE);

    // Return no of bytes written.
    return bytes_written;

}

// Function to create persistent pipe.
int create_persistent_pipe (struct exec_context *current, int *fd) {

    /**
     *  TODO:: Implementation of PPipe Create
     *
     *  Find two free file descriptors.
     *  Create two file objects for both ends by invoking the alloc_file() function.
     *  Create ppipe_info object by invoking the alloc_ppipe_info() function and
     *      fill per process and global info fields.
     *  Fill the fields for those file objects like type, fops, etc.
     *  Fill the valid file descriptor in *fd param.
     *  On success, return 0.
     *  Incase of Error return valid Error code.
     *      -ENOMEM: If memory is not enough.
     *      -EOTHERS: Some other errors.
     *
     */


    // This is similar to what was done in normal pipe
    struct file* ppipe_0 = alloc_file();
    if (ppipe_0 == NULL){
        return -ENOMEM;
    }
    struct file* ppipe_1 = alloc_file();
    if (ppipe_1 == NULL){
        return -ENOMEM;
    }
    struct ppipe_info *ppipe = alloc_ppipe_info();
    if (ppipe == NULL){
        return -ENOMEM;
    }
    
    ppipe->ppipe_per_proc[0].pid = current->pid;
    ppipe->ppipe_per_proc[0].read_open = 1;
    ppipe->ppipe_per_proc[0].write_open = 1;

    
    ppipe_0->ppipe = ppipe;
    ppipe_1->ppipe = ppipe;

    ppipe_0->type = PPIPE;
    ppipe_0->mode = O_READ;
    ppipe_0->ref_count = 1;
    ppipe_0->fops->read = ppipe_read;
    ppipe_0->fops->close = ppipe_close;
    
    ppipe_1->type = PPIPE;
    ppipe_1->mode = O_WRITE;
    ppipe_1->ref_count = 1;
    ppipe_1->fops->write = ppipe_write;
    ppipe_1->fops->close = ppipe_close;

    int i;
    for(i = 0; i < MAX_OPEN_FILES; i++){
        if (current->files[i] == NULL){
            fd[0] = i;
            break;
        }
    }

    if (i == MAX_OPEN_FILES){
        return -EOTHERS;
    }

    current->files[fd[0]] = ppipe_0;

    for (i = fd[0]; i < MAX_OPEN_FILES; i++){
        if (current->files[i] == NULL){
            fd[1] = i;
            break;
        }
    }

    if (i == MAX_OPEN_FILES){
        return -EOTHERS;
    }

    current->files[fd[1]] = ppipe_1;

    // Simple return.
    return 0;

}
