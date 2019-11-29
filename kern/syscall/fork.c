#include <types.h>
#include <spl.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>
#include <vnode.h>
#include <lib.h>
#include <copyinout.h>
#include <thread.h>
#include <limits.h>
#include <kern/syscall.h>
#include <kern/errno.h>
#include <kern/wait.h>
#include <mips/trapframe.h>
#include <kern/unistd.h>
#include <syscall.h>
#include <cdefs.h> /* for __DEAD */
#include <synch.h>
#include <vfs.h>
#include <vnode.h>

#define O_RDONLY      0
#define INVAL_PTR (void *)0x40000000  
#define KERN_PTR (void *)0x80000000  
pid_t sys_getpid(void) {
  //return the id of the current process
  return curproc->proc_pid;
}

int sys_waitpid(pid_t pid, int *stat_loc, int options, pid_t *num_ret) {
  //Check badcalls
  //PID cannot be zero.
  if (pid == 0) {
    *num_ret = -1;
    return ECHILD;
  }

  //Check if the there is no option
  if (options != 0) {
  *num_ret = -1;
  return EINVAL;
  }

  //Check if the pid is in the range
  if((unsigned)pid > array_num(procs) || (int)pid <= 0) {
    *num_ret = -1;
    return ECHILD;
  }
  lock_acquire(proc_lock);
  struct proc_helper *temp_helper = array_get(procs, (unsigned)pid);
  //Check if the process is done
  //If it's done, free it. Otherwise, wait its done
  if(!temp_helper->done) {
    cv_wait(temp_helper->proc->proc_cv, proc_lock);
  }
  lock_release(proc_lock);
  *num_ret = pid;
  *stat_loc = (temp_helper->exitcode);
  //Free the process
  array_set(procs, (unsigned)pid, NULL);
  kfree(temp_helper);
  return 0;
}


//fork() creates a new process by duplicating the calling process.
//The new process is referred to as the child process.
//The calling process is referred to as the parent process.
int sys_fork(struct trapframe *tf, pid_t *num_ret) {
  struct proc *childproc = proc_create_runprogram("children");
  //Check if the childproc is created successfully
  if (childproc == NULL){
    return ENOMEM;
  }
  //Set the parent id of the new process that is just forked as the current proc
  struct proc_helper *my_helper = kmalloc(sizeof(struct proc_helper));
  my_helper->proc = childproc;
  my_helper->parent_pid = curproc->proc_pid;
  my_helper->done = 0;
  unsigned index_ret;
  //Add new process to the process array(table)
  array_add(procs, my_helper, &index_ret);
  KASSERT(array_get(procs, index_ret) == my_helper);
  //Set the id of new process as its index in array
  childproc->proc_pid = (pid_t)index_ret;

  struct trapframe *childtf = kmalloc(sizeof(struct trapframe));

  //copy the trapframe
  memcpy(childtf,tf,sizeof(struct trapframe));
  //Copy the filetable
  memcpy(childproc->p_filetable, curproc->p_filetable, sizeof(struct file));
  for(int i = 0; i < OPEN_MAX; i++) {
    if(childproc->p_filetable[i] != NULL) {
      childproc->p_filetable[i]->count++;
    }
  }
  //Copy the addrspace
  struct addrspace *childas;
  as_copy(proc_getas(), &childas);
  childproc->p_addrspace = childas;

  //fork process
  int err = thread_fork("child", childproc, &enter_forked_process, (void *)childtf, (unsigned long)childas);
  if(err) {
    return err;
  }
  //return the id of the new process
  *num_ret = childproc->proc_pid;
  return 0;
}



void sys__exit(int exitcode){
  struct proc_helper* helper = array_get(procs, (unsigned)curproc->proc_pid);
  struct proc_helper* temp;
  pid_t t;
  int tt;
  //Go through the whole process table and check the child process
  //After found the child process, call waitpid
  for(unsigned i = 1; i < array_num(procs); i++) {
    temp = array_get(procs, i);
    if(temp != NULL){
      if(temp->proc != NULL ){
        if(temp->parent_pid == curproc->proc_pid)
          sys_waitpid(temp->proc->proc_pid, &tt, 0, &t);
      }
    }
  }
  //Keep atomic
  lock_acquire(proc_lock);
  //Set the process as done and record its exitcode
  helper->done = 1;
  helper->exitcode = _MKWAIT_EXIT(exitcode);
  //broadcast the conditional variable from wait queue
  cv_broadcast(curproc->proc_cv, proc_lock);
  lock_release(proc_lock);

  struct proc * my_proc = curproc;
  proc_remthread(curthread);
  proc_addthread(kproc, curthread);
  //destory the process before exit
  proc_destroy(my_proc);
  my_proc = NULL;
  thread_exit();
}

int sys_execv(const char *prog, char **args){
	
	int errReturn = 0; //Return value check to see if there are invalid returns
	struct vnode *v;
	if (!prog || (void *)prog >= INVAL_PTR || (void *)args <= INVAL_PTR || args == NULL || (void*) args >= KERN_PTR ) {
		return EFAULT; //check that the pointer addresses are valid
	}

	//Copy program name to this array
    	char *pname = (char *) kmalloc(strlen(prog)+1);
	size_t sizingForPath;  
	
	errReturn = copyinstr((const_userptr_t)prog, pname, PATH_MAX, &sizingForPath);
  	if(errReturn)
    	{
        	kfree(pname);
        	return errReturn;
    	}

    	int argc = 0; //argument counter

	//Count how many arguments are in the args	
    	while(args[argc] != NULL)
    	{
		if((void *)args[argc] >= INVAL_PTR){
			return EFAULT; //if the argument pointers point to an invalid memory area
		}
        	argc = argc + 1;
    	} 

	//The copied in arguments		
	char **argv = (char **)kmalloc((argc + 1) * sizeof(char *));

    	for(int i = 0; i < argc; i++)
    	{
        	size_t len = strlen(args[i]) + 1;
        	argv[i] = (char *)kmalloc(len * sizeof(char));
        	errReturn = copyin((const_userptr_t)args[i],argv[i],len * sizeof(char));
        	if(errReturn)
        	{
            		kfree(pname);
			deallocateMemory(i + 1, argv); //case where memory is already alocated
            		return errReturn;
        	}        
    	}
   
 
	argv[argc] = NULL;
	errReturn = vfs_open(pname, O_RDONLY, 0, &v);

	if (errReturn) {
        	kfree(pname);
		deallocateMemory(argc, argv);
        	vfs_close(v);
		return errReturn;
	}
	
	//create address space
	struct addrspace *as = as_create();
    	if(as == NULL)
    	{
        	kfree(pname);
		deallocateMemory(argc, argv);
        	vfs_close(v);
        	return ENOMEM;
    	}

    	struct addrspace *oldas = proc_setas(as);
	//to delete addressspace when needed

	vaddr_t entrypoint, stackptr;
   
    	errReturn = load_elf(v,&entrypoint);
	if(errReturn)
    	{
        	kfree(pname);
		deallocateMemory(argc, argv);
        	as_deactivate();
        	as = proc_setas(oldas);
        	as_destroy(as);
        	vfs_close(v);
        	return errReturn;
    	}


	vfs_close(v);
	kfree(pname);

    	errReturn = as_define_stack(as,&stackptr);
	if(errReturn)
    	{	
		deallocateMemory(argc,argv);
        	as_deactivate();
        	as = proc_setas(oldas);
        	as_destroy(as);
        	return errReturn;
    	}

       

    	vaddr_t *argumentPtrs = (vaddr_t *)kmalloc((argc + 1) * sizeof(vaddr_t));
	for(int i = argc-1; i >= 0; i--)
    	{
        	size_t curArgLen = strlen(argv[i]) + 1; 
		size_t argLen = ROUNDUP(curArgLen,4);
        	stackptr -= (argLen * sizeof(char));
        	errReturn = copyout((void *) argv[i], (userptr_t)stackptr, curArgLen);
        	argumentPtrs[i] = stackptr;        
    	} 	   
        
    	argumentPtrs[argc] = (vaddr_t)NULL;
    
    	for(int i = argc; i >= 0; i--)
    	{
        	stackptr -= sizeof(vaddr_t);
        	errReturn = copyout((void *) &argumentPtrs[i], ((userptr_t)stackptr),sizeof(vaddr_t));
    	}	
    
    	vaddr_t argumentvPtr = stackptr;
    	vaddr_t offset = ROUNDUP(USERSTACK - stackptr,8);
    	stackptr = USERSTACK - offset;

	deallocateMemory(argc, argv);
	as_destroy(oldas);
    	as_activate();
	enter_new_process(argc, (userptr_t)argumentvPtr, NULL, stackptr, entrypoint);
	
	return EINVAL;
}

void deallocateMemory(int argc, char **argv){
	
	for(int i = 0; i < argc; ++i)
        {
        	kfree(argv[i]);
        }
	kfree(argv);
}
