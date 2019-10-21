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

#define SEEK_SET      0      /* Seek relative to beginning of file */
#define SEEK_CUR      1      /* Seek relative to current position in file */
#define SEEK_END      2      /* Seek relative to end of file */

off_t
sys_lseek(int fd, off_t pos, int whence, off_t *retval)
{
	
	*retval = -1; //To clear all the bits
	int err;
	off_t futureOffset;
	struct stat *stat = kmalloc(sizeof(struct stat));
	if (whence < 0 || whence > 2) { //Here whence is invalid
        	return EINVAL; 
	}
	if(curproc->p_filetable == NULL ||(fd < 0 || fd >= OPEN_MAX)|| curproc->p_filetable[fd] == NULL){
		return EBADF;
	}
	//After passing those checks here is our file	
	struct file *file = curproc->p_filetable[fd];

	lock_acquire(file->file_lock);

	//see if fd is a file that supports seeking
	if (!VOP_ISSEEKABLE(file->file_vnode)) {
        	lock_release(file->file_lock);
        	return ESPIPE;
    	}

  	switch (whence) {
        case SEEK_SET:
        	futureOffset = pos;
        	break;
        case SEEK_CUR:
        	futureOffset = file->file_offset + pos;
        	break;
        case SEEK_END:
		err = VOP_STAT(file->file_vnode, stat);
		if (err){
			lock_release(file->file_lock);
			return err;
		}
		futureOffset = stat->st_size + pos;
		break;
    	}  

	if (futureOffset < 0) {
		lock_release(file->file_lock);
        	return EINVAL; //Seek position cannot be negative
    	}

	
	file->file_offset = futureOffset;
	lock_release(file->file_lock);
	*retval = futureOffset;

	return 0;
}
