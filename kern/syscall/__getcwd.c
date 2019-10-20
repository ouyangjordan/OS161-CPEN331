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
#include <uio.h>

int sys___getcwd(userptr_t buf, size_t buflen, int *retval){


	struct uio user;
	struct iovec vec;

	vec.iov_ubase = (userptr_t)buf;
   	vec.iov_len = buflen;
    	user.uio_iov = &vec;
    	user.uio_iovcnt = 1;
    	user.uio_resid = buflen;
    	user.uio_offset = 0;
    	user.uio_segflg = UIO_USERSPACE;
    	user.uio_rw = UIO_READ;
    	user.uio_space = curproc->p_addrspace;
	*retval = buflen - user.uio_resid;
	return 0;
}
