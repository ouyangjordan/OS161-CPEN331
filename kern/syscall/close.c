#include <cdefs.h> /* for __DEAD */
#include <kern/unistd.h>
#include <types.h>
#include <vfs.h>
#include <file.h>
#include <kern/fcntl.h>
#include <vnode.h>
#include <syscall.h>
#include <synch.h>
#include <current.h>
#include <limits.h>
#include <kern/errno.h>
#include <proc.h>


int sys_close(int fd, int32_t *num_ret){

    if(fd <0 || fd>OPEN_MAX ||curproc->p_filetable[fd] == NULL){
      *num_ret = -1;
      return EBADF;
    }

    lock_acquire(curproc->p_filetable[fd]->file_lock);

    curproc->p_filetable[fd]->count--;
    if(curproc->p_filetable[fd]->count > 0){
      lock_release(curproc->p_filetable[fd]->file_lock);
    }
    else{
      vfs_close(curproc->p_filetable[fd]->file_vnode);
      curproc->p_filetable[fd]->file_vnode = NULL;
      curproc->p_filetable[fd]->file_offset = 0;
      curproc->p_filetable[fd]->file_flag = 0;
      lock_release(curproc->p_filetable[fd]->file_lock);
      file_cleanup(curproc->p_filetable[fd]);
      curproc->p_filetable[fd] = NULL;
    }
    *num_ret = 0;
    return 0;

}
