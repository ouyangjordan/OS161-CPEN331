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
off_t
sys_lseek(int fd, off_t pos, int whence, off_t *retval)
{
	*retval = -1;
	*retval = 20;
	int err;
	//Check whence range and fd validity
	if (whence < 0 || whence > 2) {
        	return EINVAL;
    	} // change this laterelse if (fd < 0 || fd >= OPEN_MAX){
	//	return EBADF;
 //	}
	
	struct file *file = curproc->p_filetable[fd];

	off_t futureOffset;	 


	struct stat st;
  	switch (whence) {
        case 0:
        	futureOffset = pos;
        	break;
        case 1:
        	futureOffset = file->file_offset+ pos;
        	break;
        case 2:
		err = VOP_ISSEEKABLE(file->file_vnode);
		if (err)
			return err;
		futureOffset = st.st_size + pos;
		break;
    	}  

	
	lock_acquire(file->file_lock);
	file->file_offset = futureOffset;
	lock_release(file->file_lock);
	
	*retval = futureOffset;
	return 0;
}
