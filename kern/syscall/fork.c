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

  if (pid == 0) {
    *num_ret = -1;
    return ECHILD;
  }

  //Check if the there is no option
  if (options != 0) {
  *num_ret = -1;
  return EINVAL;
  }

  //Check if the pid exists
  if((unsigned)pid > array_num(procs) || (int)pid <= 0) {
    *num_ret = -1;
    return ECHILD;
  }
  lock_acquire(proc_lock);
  int* done = array_get(donearray, (unsigned)pid);
  if(!done) {
    struct proc *childproc;
    childproc = array_get(procs,(unsigned)pid);
    cv_wait(childproc->proc_cv, proc_lock);
  }
  lock_release(proc_lock);
  *num_ret = pid;
  *stat_loc = *((int *)array_get(exitcodearray, (unsigned)pid));
  array_set(procs, (unsigned)pid, NULL);
  array_set(exitcodearray, (unsigned)pid, NULL);
  array_set(donearray, (unsigned)pid, NULL);
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
  unsigned index_ret;
  array_add(procs, childproc, &index_ret);
  array_add(exitcodearray, NULL, &index_ret);
  array_add(donearray, NULL, &index_ret);
  KASSERT(array_get(procs, index_ret) == childproc);
  childproc->proc_pid = (pid_t)index_ret;
  //pid_t pid = kproc->proc_pid;
  KASSERT(array_get(procs, 0) == NULL);
  KASSERT(array_get(procs, 1) == kproc);
  childproc->parent_pid = curproc->proc_pid;

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



// void
// entrance(void *trapframe, unsigned long addrspace)
// {
//   strcut addrspace *my_as = (struct addrspace *)addrspace;
//   //allocate on the kernel's stack
//     struct trapframe *current_tf = (struct trapframe *)trapframe;
//
//     current_tf.tf_v0 = 0; //forked children returns 0
//     current_tf.tf_a3 = 0; //no error
//     current_tf.tf_epc += 4; //adjust program counter
//     proc_setas(my_as);
//     as_activate();
//     mips_usermode(&current_tf);
//     struct trapframe final_tf;
//     memcpy(&final_tf, current_tf, sizeof(struct trapframe));
//     kfree(trapframe);
//     mips_usermode(&final_tf);
// }



void sys__exit(int exitcode){
  struct proc *proc;
  int* temp_exit = kmalloc(sizeof(int));
  int* temp_done = kmalloc(sizeof(int));
  pid_t id = curproc->parent_pid;
  (void)id;
  for(unsigned i = 1; i < array_num(procs); i++) {
    proc = array_get(procs, i);
    if(proc != NULL && proc->parent_pid == curproc->proc_pid){
        proc->parent_pid = 0;
    }
  }
  if(curproc->parent_pid == 0){
    array_set(procs, (unsigned)curproc->proc_pid, NULL);
    array_set(exitcodearray, (unsigned)curproc->proc_pid, NULL);
    array_set(donearray, (unsigned)curproc->proc_pid, NULL);
  }
  else{
    lock_acquire(proc_lock);
  curproc->done = 1;
  //add exitcode to exitcodearray
  *temp_exit = _MKWAIT_EXIT(exitcode);
  array_set(exitcodearray, (unsigned)curproc->proc_pid, temp_exit);
  KASSERT(array_get(exitcodearray, (unsigned)curproc->proc_pid) == temp_exit);

  //add done_status to donearray
  *temp_done = curproc->done;
  array_set(donearray, (unsigned)curproc->proc_pid, temp_done);
  KASSERT(array_get(donearray, (unsigned)curproc->proc_pid) == temp_done);
  cv_broadcast(curproc->proc_cv, proc_lock);
  lock_release(proc_lock);
  }
  // pid_tbl[curproc->p_id]->exited = true;
  // pid_tbl[curproc->p_id]->exitcode = _MKWAIT_EXIT(exitcode);
  //
  // pid_tbl[curproc->p_id]->id_process = NULL;
  //
  //
  // if(pid_tbl[curproc->p_id]->parentPid == 0){
  //   delete_pid(curproc->p_id);
  // }
  // else{
  //   pid_cv_broadcast(curproc->p_id);
  // }

  struct proc * my_proc = curproc;
  proc_remthread(curthread);
  proc_addthread(kproc, curthread);
  //kprintf("Get to there");
  proc_destroy(my_proc);
  my_proc = NULL;
  thread_exit();

}
