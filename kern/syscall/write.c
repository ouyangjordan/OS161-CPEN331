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
#include <uio.h>
#include <kern/iovec.h>



ssize_t sys_write(int fd, const void *buf, size_t nbytes, int32_t *num_ret){

  //require a uio and a base buffer vector
  struct uio myuio;
  struct iovec i;
  int result;
  int diff;


  //check flag
  int flag_checked = curproc->p_filetable[fd]->file_flag & O_ACCMODE;
  if(flag_checked == O_RDONLY && flag_checked != O_RDWR){
      *num_ret = -1;
      return EBADF;
  }

  //check if buf if valid
  if(buf == NULL){
      *num_ret = -1;
      kprintf("num_ret4: %d\n", *num_ret);
      return EFAULT;
  }

  //check if fd is valid
  if(fd < 0 || fd >= OPEN_MAX || curproc->p_filetable[fd] == NULL){
      *num_ret = -1;
      kprintf("num_ret5: %d\n", *num_ret);
      return EBADF;
  }


  //keep atomic
  lock_acquire(curproc->p_filetable[fd]->file_lock);
  //initialization for uio
  uio_kinit(&i, &myuio,
  	       (void *)buf, nbytes, curproc->p_filetable[fd]->file_offset, myuio.uio_rw);
   //set the segflg to the user space
    myuio.uio_segflg = UIO_USERSPACE;
    //set the mode to write
    myuio.uio_rw = UIO_WRITE;
    //set the space to the current proc's space
    myuio.uio_space = curproc->p_addrspace;

   //implement vop_write
   result = VOP_WRITE(curproc->p_filetable[fd]->file_vnode, &myuio);

   //check if the result is wrong
   if(result){
     lock_release(curproc->p_filetable[fd]->file_lock);
     return result;
   }

   diff = myuio.uio_offset - curproc->p_filetable[fd]->file_offset;
   *num_ret = diff;
   curproc->p_filetable[fd]->file_offset = myuio.uio_offset;
   lock_release(curproc->p_filetable[fd]->file_lock);
   return 0;
}
