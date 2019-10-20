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
sys_chdir(const char *filepath){

	if(filepath == NULL)
		return EFAULT;

	char *newPath = kmalloc(__PATH_MAX);
	int error = copyinstr((const_userptr_t)filepath, newPath, __PATH_MAX, NULL);
	if (error) {
		return error;
	}

	int result = vfs_chdir((char *) filepath);


    	if (result) {
        	return result;
    	}
	return 0;
}
