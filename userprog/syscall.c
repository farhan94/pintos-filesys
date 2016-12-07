#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

typedef int pid_t;

static void syscall_handler(struct intr_frame *);
void check_bad_ptr(void* arg_ptr);
bool mkdir(const char *dir);

get_user (const uint8_t *uaddr)
{
  int result;
  asm ("movl $1f, %0; movzbl %1, %0; 1:"
       : "=&a" (result) : "m" (*uaddr));
  return result;
}
 

void
syscall_init(void) {
    intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler(struct intr_frame *f) {
    // printf("stack pointer of thread is %d\n", thread_current()->stack);
    // printf("intr_frame stack pointer is %d\n", f->esp);
    // first check if sp and the first arg are valid
    if (!is_user_vaddr(f->esp) || !is_user_vaddr(f->esp + 4)) {
        exit(-1);
    }
    if (!pagedir_get_page(thread_current()->pagedir, f->esp) || !pagedir_get_page(thread_current()->pagedir, f->esp + 4)) {
        exit(-1);
    }
    // int x =0;
    int syscall_number = *((int *) f->esp);
    // printf("%s in syscall handler witch call %d\n", thread_current()->name, syscall_number);
    switch (syscall_number) {
        case SYS_HALT: {
            halt();
            break;
        }
        case SYS_EXIT: {
            int status = *(int *) (f->esp + 4);
            exit(status);
            break;
        }
        case SYS_EXEC: {
            // printf("%s in exec\n", thread_current()->name);
            //TODO: maybe check if command string is larger than a pagesize
            //TODO: check non empty string and dont start with space?
            check_bad_ptr(*(void**)(f->esp + 4));
            char* cmd_line = *(char **) (f->esp + 4);
            // void *vp = *(void **) (f->esp + 4);
            // void *ptr = pagedir_get_page(thread_current()->pagedir, vp);
            // if (!ptr){
            //  // printf("asdfasdf\n\n\n\n\n");
            //   exit(-1);
            //   //f->eax = -1;
            //   break;
            // }
            f->eax = exec(cmd_line);
            break;
        }
        case SYS_WAIT: {
            pid_t pid = *(int *) (f->esp + 4);
            f->eax = wait(pid);
            break;
        }
        case SYS_CREATE: {
            check_bad_ptr(*(void**)(f->esp + 4));
            char *file = *(char **) (f->esp + 4);
            // void *vp = *(void **) (f->esp + 4);
            // void *ptr = pagedir_get_page(thread_current()->pagedir, vp);
            // if (!ptr){
            //  // printf("asdfasdf\n\n\n\n\n");
            //   exit(-1);
            //   //f->eax = -1;
            //   break;
            // }
            unsigned initial_size = *(unsigned *) (f->esp + 8);
            f->eax = create(file, initial_size);
            break;
        }
        case SYS_REMOVE: {
            check_bad_ptr(*(void**)(f->esp + 4));
            char *file = *(char **) (f->esp + 4);
            f->eax = remove(file);
            break;
        }
        case SYS_OPEN: {
            check_bad_ptr(*(void**)(f->esp + 4));
            // void *vp = *(void **) (f->esp + 4);
            // void *ptr = pagedir_get_page(thread_current()->pagedir, vp);
            // if (!ptr){
            //  // printf("asdfasdf\n\n\n\n\n");
            //   exit(-1);
            //   //f->eax = -1;
            //   break;
            // }
            char *file = *(char **) (f->esp + 4);
            f->eax = open(file);
            break;
        }
        case SYS_FILESIZE: {
            int fd = *(int *) (f->esp + 4);
            f->eax = filesize(fd);
            break;
        }
        case SYS_READ: {
            check_bad_ptr(*(void**)(f->esp + 8));
            int fd = *(int *) (f->esp + 4);
            void *buffer = *(char **) (f->esp + 8);
            unsigned size = *(unsigned *) (f->esp + 12);
            f->eax = read(fd, buffer, size);
            break;
        }
        case SYS_WRITE: {
            check_bad_ptr(*(void**)(f->esp + 8));
            int fd = *(int *) (f->esp + 4);
            void *buffer = *(char **) (f->esp + 8);
            unsigned size = *(unsigned *) (f->esp + 12);
            f->eax = write(fd, buffer, size);
            break;

        }
        case SYS_SEEK: {
            int fd = *(int *) (f->esp + 4);
            unsigned position = *(unsigned *) (f->esp + 8);
            seek(fd, position);
            break;
        }
        case SYS_TELL: {
            int fd = *(int *) (f->esp + 4);
            f->eax = tell(fd);
            break;
        }
        case SYS_CLOSE: {
            int fd = *(int *) (f->esp + 4);
            close(fd);
            break;
        }
        case SYS_MKDIR:{
            check_bad_ptr(*(void**)(f->esp + 4));
            char* dir = *(char **) (f->esp + 4);
            f->eax = mkdir((const char*)dir);
            break;
        }
   //     case SYS_
    }
}



static struct fd_elem *get_fd_element(int fd) {
//    struct list *fd_list = &thread_current()->fd_list;
//    struct list_elem *ptr = list_begin(fd_list);
//    struct list_elem *end = list_end(fd_list);
//
//    struct fd_elem *element = NULL;
//    while(ptr != end){
//        element = list_entry(ptr, struct fd_elem, element);
//        if(element->fd == fd){
//            break;
//        }
//        ptr = list_next(ptr);
//    }
//    return element;

    struct list_elem *e;
    struct fd_elem *element = NULL;
    struct list *fd_list = &thread_current()->fd_list;

    for (e = list_begin(fd_list); e != list_end(fd_list); e = list_next(e)) {
        struct fd_elem *tmp = list_entry(e, struct fd_elem, element);
        if(tmp->fd == fd){
            element = tmp;
            break;
        }
    }
    return element;
}

void halt(void) {
    /*Terminates Pintos by calling shutdown_power_off() 
    (declared in "threads/init.h"). This should be seldom used, 
    because you lose some information about possible deadlock situations, etc. */
    shutdown_power_off();
}

void exit(int status) {
    /*Terminates the current user program, returning status to the kernel. 
    If the process's parent waits for it (see below), this is the status that 
    will be returned. Conventionally, a status of 0 indicates success and nonzero values
    indicate errors. */

    //TODO: store status eventually

    thread_exit(status);
}

pid_t exec(const char *cmd_line) {
    /*Runs the executable whose name is given in cmd_line, passing any given arguments, 
    and returns the new process's program id (pid). Must return pid -1, which otherwise 
    should not be a valid pid, if the program cannot load or run for any reason. Thus, the 
    parent process cannot return from the exec until it knows whether the child process 
    successfully loaded its executable. You must use appropriate synchronization to ensure this.*/
    return process_execute(cmd_line);
}

int wait(pid_t pid) {
/*    Waits for a child process pid and retrieves the child's exit status.

    If pid is still alive, waits until it terminates. Then, returns the status that pid passed 
    to exit. If pid did not call exit(), but was terminated by the kernel (e.g. killed due to an
    exception), wait(pid) must return -1. It is perfectly legal for a parent process to wait for
    child processes that have already terminated by the time the parent calls wait, but the
    kernel must still allow the parent to retrieve its child's exit status, or learn that the
    child was terminated by the kernel.

    wait must fail and return -1 immediately if any of the following conditions is true:

        pid does not refer to a direct child of the calling process. pid is a direct child of
         the calling process if and only if the calling process received pid as a return value 
         from a successful call to exec.

        Note that children are not inherited: if A spawns child B and B spawns child process C,
         then A cannot wait for C, even if B is dead. A call to wait(C) by process A must fail. 
         Similarly, orphaned processes are not assigned to a new parent if their parent process
         exits before they do.

        The process that calls wait has already called wait on pid. That is, a process may wait
         for any given child at most once. 

    Processes may spawn any number of children, wait for them in any order, and may even exit
     without having waited for some or all of their children. Your design should consider all 
     the ways in which waits can occur. All of a process's resources, including its struct thread,
      must be freed whether its parent ever waits for it or not, and regardless of whether
       the child exits before or after its parent.

    You must ensure that Pintos does not terminate until the initial process exits. 
    The supplied Pintos code tries to do this by calling process_wait() (in "userprog/process.c") 
    from main() (in "threads/init.c"). We suggest that you implement process_wait() according to
     the comment at the top of the function and then implement the wait system call in terms of
      process_wait().

    Implementing this system call requires considerably more work than any of the rest.*/

    return process_wait(pid);
}


bool create(const char *file, unsigned initial_size) {
    /*  Creates a new file called file initially initial_size bytes in size.
      Returns true if successful, false otherwise. Creating a new file does
       not open it: opening the new file is a separate operation which would
       require a open system call. */
    return filesys_create(file, initial_size);
}

bool remove(const char *file) {
    /*  Deletes the file called file. Returns true if successful, false otherwise.
      A file may be removed regardless of whether it is open or closed, and removing
       an open file does not close it. See Removing an Open File, for details.*/
    return filesys_remove(file);
}

int open(const char *file) {
    /*  Opens the file called file. Returns a nonnegative integer handle called a "file descriptor"
      fd), or -1 if the file could not be opened.

      File descriptors numbered 0 and 1 are reserved for the console: fd 0 (STDIN_FILENO) is
      standard input, fd 1 (STDOUT_FILENO) is standard output. The open system call will
      never return either of these file descriptors, which are valid as system call arguments
       only as explicitly described below.

      Each process has an independent set of file descriptors. File descriptors are not inherited
      by child processes.

      When a single file is opened more than once, whether by a single process or different
      processes, each open returns a new file descriptor. Different file descriptors for
       a single file are closed independently in separate calls to close and they do not
       share a file position.*/
//    struct file *openfile = filesys_open(file);
//    if (openfile == NULL) {
//        return -1;
//    }
//    struct fd_elem *fd_elem = malloc(sizeof(struct fd_elem));
//
//    unsigned updated_fd = thread_current()->next_file;
//    thread_current()->next_file = updated_fd;
//    fd_elem->fd = updated_fd;
//    fd_elem->file = openfile;
//    list_push_back(&thread_current()->fd_list, &fd_elem->element);
//    return updated_fd;

    struct fd_elem *fe = (struct fd_elem*) malloc(sizeof(struct fd_elem));
    struct file *f = filesys_open(file);
    if(f == NULL){
        return -1;
    }
    fe->fd = thread_current()->next_file;
    thread_current()->next_file = thread_current()->next_file + 1;
    list_push_back(&thread_current()->fd_list, &fe->element);
    return fe->fd;
}

int filesize(int fd) {
    /* Returns the size, in bytes, of the file open as fd. */
    struct fd_elem *fd_elem = get_fd_element(fd);
    if (fd_elem == NULL) {
        return -1;
    } else {
        return file_length(fd_elem->file);
    }
}

int read(int fd, void *buffer, unsigned size) {
    /*   Reads size bytes from the file open as fd into buffer. Returns the number of
       bytes actually read (0 at end of file), or -1 if the file could not be read
       (due to a condition other than end of file). Fd 0 reads from the keyboard
        using input_getc(). */
    struct fd_elem *fd_elem = get_fd_element(fd);
    if (fd_elem == NULL) {
        // printf("here\n");
        return -1;
    } else {
       // file_deny_write(fd_elem->file);
        return file_read(fd_elem->file, buffer, size);
    }
}

int write(int fd, const void *buffer, unsigned size) {
    /*   Writes size bytes from buffer to the open file fd. Returns the number of
       bytes actually written, which may be less than size if some bytes could not be written.

       Writing past end-of-file would normally extend the file, but file growth
       is not implemented by the basic file system. The expected behavior is to
        write as many bytes as possible up to end-of-file and return the actual
         number written, or 0 if no bytes could be written at all.

       Fd 1 writes to the console. Your code to write to the console should write
       all of buffer in one call to putbuf(), at least as long as size is not bigger
       than a few hundred bytes. (It is reasonable to break up larger buffers.)
       Otherwise, lines of text output by different processes may end up interleaved
       on the console, confusing both human readers and our grading scripts.*/

    if (fd == 0) {
        return -1;
    } else if (fd == 1) {
        char *b = (char *) buffer;
        //printf("%d \n", size);
        putbuf(b, size);
        return (int) size;
    } else if(get_fd_element(fd) != NULL){
        return file_write(get_fd_element(fd)->file, buffer, size);
    }

    return -1;
}

void seek(int fd, unsigned position) {
    /*  Changes the next byte to be read or written in open file fd to position,
      expressed in bytes from the beginning of the file. (Thus, a position of 0
      is the file's start.)

      A seek past the current end of a file is not an error. A later read obtains
      0 bytes, indicating end of file. A later write extends the file, filling any
     unwritten gap with zeros. (However, in Pintos files have a fixed length until
     project 4 is complete, so writes past end of file will return an error.)
     These semantics are implemented in the file system and do not require any
     special effort in system call implementation.*/
    struct fd_elem *fd_elem = get_fd_element(fd);
    if (fd_elem == NULL) {
        return -1;
    } else {
        file_seek(fd_elem->file, position);
    }
}

int tell(int fd) {
    /*  Returns the position of the next byte to be read or written in open file fd,
       expressed in bytes from the beginning of the file. */
    struct fd_elem *fd_elem = get_fd_element(fd);
    if (fd_elem == NULL) {
        return -1;
    } else {
        file_tell(fd_elem->file);
    }
    return -1;
}

void close(int fd) {
    /* Closes file descriptor fd. Exiting or terminating a process implicitly closes
     all its open file descriptors, as if by calling this function for each one. */
     
}

void check_bad_ptr(void* arg_ptr) {
    // printf("checking address %x\n", (unsigned char*)arg_ptr);
    if (!arg_ptr) {
        exit(-1);
        // printf("arg ptr is just null\n");
        return;
    }
    if (!is_user_vaddr(arg_ptr)) {
        // printf("invalid is_user_vaddr\n");
        exit(-1);
    }
    if (!pagedir_get_page(thread_current()->pagedir, arg_ptr)) {
        // printf("%s: not valid page\n", thread_current()->name);
        exit(-1);
    }
}

/*
 * Changes the current working directory of the process to dir,
 * which may be relative or absolute. Returns true if successful, false on failure.
 */
bool chdir(const char *dir) {

}

/*
 * Creates the directory named dir, which may be relative or absolute.
 * Returns true if successful, false on failure. Fails if dir already
 * exists or if any directory name in dir, besides the last, does not already exist.
 * That is, mkdir("/a/b/c") succeeds only if "/a/b" already exists and "/a/b/c" does not.
 */
bool mkdir(const char *dir){
    return filesys_create(dir, 0, true);
}

/*
 * Reads a directory entry from file descriptor fd, which must represent a directory.
 * If successful, stores the null-terminated file name in name, which must have room for
 * READDIR_MAX_LEN + 1 bytes, and returns true. If no entries are left in the directory,
 * returns false.

"." and ".." should not be returned by readdir.

If the directory changes while it is open, then it is acceptable for some entries not
 to be read at all or to be read multiple times. Otherwise, each directory entry should be
 read once, in any order.

READDIR_MAX_LEN is defined in "lib/user/syscall.h". If your file system supports longer
 file names than the basic file system, you should increase this value from the default of 14.
 */
bool readdir(int fd, char *name){

}

/*
 * Returns true if fd represents a directory, false if it represents an ordinary file.
 */
bool isdir(int fd){

}

/*
 * Returns the inode number of the inode associated with fd, which may represent
 * an ordinary file or a directory.

An inode number persistently identifies a file or directory. It is unique during
 the file's existence. In Pintos, the sector number of the inode is suitable for
 use as an inode number.
 */
int inumber(int fd){

}
