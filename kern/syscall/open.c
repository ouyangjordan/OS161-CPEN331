#include <cdefs.h> /* for __DEAD */
#include <kern/unistd.h>
#include <types.h>
#include <vfs.h>
#include <file.h>
#include <kern/fcntl.h>
#include <vnode.h>
#include <syscall.h>
#include <synch.h>
#include <limits.h>
#include <kern/errno.h>
#include <current.h>
#include <proc.h>
#include <copyinout.h>



int sys_open(const char *filename, int flags, mode_t mode, int32_t *retval){

    int result;
    int fd = 0;
    int new_flag;
    size_t value;
    char new_filename[PATH_MAX];
    new_flag = flags & O_ACCMODE;
    //types of error
    if(new_flag != O_RDONLY && new_flag != O_WRONLY && new_flag != O_RDWR ){
        *retval = -1;
        return EINVAL;
    }

    struct vnode *vn = NULL;

    //check filename
    if(filename == NULL){
        *retval = -1;
        return EFAULT;
    }

    //find a location in filetable for the target file
    for(int index = 0; index < OPEN_MAX; index++){
        if(curproc->p_filetable[index] == NULL){
            fd = index;
            curproc->p_filetable[fd] = file_create();//ftable initialization
            break;
        }
        if(index == OPEN_MAX - 1){
            *retval = -1;
            return EMFILE;
        }
    }

    //keep atomic
   lock_acquire(curproc->p_filetable[fd]->file_lock);

    result = copyinstr((const_userptr_t) filename, new_filename, PATH_MAX, &value);
    if(result != 0){
        *retval = -1;
        lock_release(curproc->p_filetable[fd]->file_lock);
        return EINVAL;
    }

    result = vfs_open(new_filename, flags, mode, &vn);//find the vnode based on the directory
    //keep atomic
    if(result != 0){
        *retval = -1;
        lock_release(curproc->p_filetable[fd]->file_lock);
        return EINVAL;
    }

    //steps for update variables
    curproc->p_filetable[fd]->file_vnode = vn;
    curproc->p_filetable[fd]->count++;
    curproc->p_filetable[fd]->file_flag = flags;

    lock_release(curproc->p_filetable[fd]->file_lock);
    *retval = fd;
    return 0;


}
