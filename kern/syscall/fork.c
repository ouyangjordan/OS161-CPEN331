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
  if(!temp_helper->done) {
    cv_wait(temp_helper->proc->proc_cv, proc_lock);
  }
  lock_release(proc_lock);
  *num_ret = pid;
  *stat_loc = (temp_helper->exitcode);
  array_set(procs, (unsigned)pid, NULL);
  kfree(temp_helper);
  return 0;
}


//fork() creates a new process by duplicating the calling process.
//The new process is referred to as the child process.
//The calling process is referred to as the parent process.
int sys_fork(struct trapframe *tf, pid_t *num_ret) {
  struct proc *childproc = proc_create_runprogram("children");
  if (childproc == NULL){
    return ENOMEM;
  }
  struct proc_helper *my_helper = kmalloc(sizeof(struct proc_helper));
  my_helper->proc = childproc;
  my_helper->parent_pid = curproc->proc_pid;
  my_helper->done = 0;
  unsigned index_ret;
  array_add(procs, my_helper, &index_ret);
  KASSERT(array_get(procs, index_ret) == my_helper);
  childproc->proc_pid = (pid_t)index_ret;

  struct trapframe *childtf = kmalloc(sizeof(struct trapframe));

  //copy the trapframe
  memcpy(childtf,tf,sizeof(struct trapframe));
  memcpy(childproc->p_filetable, curproc->p_filetable, sizeof(struct file));
  for(int i = 0; i < OPEN_MAX; i++) {
    if(childproc->p_filetable[i] != NULL) {
      childproc->p_filetable[i]->count++;
    }
  }

  struct addrspace *childas;
  as_copy(proc_getas(), &childas);
  childproc->p_addrspace = childas;

  int err = thread_fork("child", childproc, &enter_forked_process, (void *)childtf, (unsigned long)childas);
  if(err) {
    return err;
  }
//(void)pid;
  *num_ret = childproc->proc_pid;
  return 0;
}



void sys__exit(int exitcode){
  struct proc_helper* helper = array_get(procs, (unsigned)curproc->proc_pid);
  struct proc_helper* temp;
  pid_t t;
  int tt;
  for(unsigned i = 1; i < array_num(procs); i++) {
    temp = array_get(procs, i);
    if(temp != NULL){
      if(temp->proc != NULL ){
        if(temp->parent_pid == curproc->proc_pid)
          sys_waitpid(temp->proc->proc_pid, &tt, 0, &t);
      }
    }
  }
    lock_acquire(proc_lock);
    helper->done = 1;

  helper->exitcode = _MKWAIT_EXIT(exitcode);

  cv_broadcast(curproc->proc_cv, proc_lock);
  lock_release(proc_lock);
  struct proc * my_proc = curproc;
  proc_remthread(curthread);
  proc_addthread(kproc, curthread);
  //kprintf("Get to there");
  proc_destroy(my_proc);
  my_proc = NULL;
  thread_exit();
}
