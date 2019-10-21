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
#include <stat.h>
int
sys_dup2(int oldfd, int newfd, int *retval){

	if (newfd < 0 || oldfd < 0 || newfd >= OPEN_MAX || oldfd >= OPEN_MAX) {
        	*retval = -1;
		return EBADF;
    	}
	*retval = newfd; //if new file is the same as old
	if (oldfd == newfd){
		return 0;
	}

	struct file *oldFile = curproc->p_filetable[oldfd];
	
	lock_acquire(oldFile->file_lock);

	curproc->p_filetable[newfd] = file_create();	
	struct file *newFile = curproc->p_filetable[newfd];
	

	
	newFile->file_vnode = oldFile->file_vnode;
	newFile->file_offset = oldFile->file_offset;
	oldFile->count++;
	newFile->count = oldFile->count;
	
	newFile->file_flag = oldFile->file_flag;
	lock_release(oldFile->file_lock);
	return 0;
}
