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

  //Check if the there is no options
  if (options != 0) {
  *num_ret = -1;
  return EINVAL;
  }
  bool flag = 0;
  unsigned index;
  //Check if the pid exists
  for(unsigned i = 0; i < curproc->childprocs->num && !flag; i++) {
    if((pid_t)array_get(curproc->childprocs, i) == pid) {
      flag = 1;
      index = i;
      break;
    }
  }
  if(!flag) {
    *num_ret = -1;
    return ECHILD;
  }
  struct proc *temp_proc = array_get(curproc->childprocs, index);
  if(!temp_proc->done) {
    lock_acquire(temp_proc->proc_lock);
    cv_wait(temp_proc->proc_cv, temp_proc->proc_lock);
    lock_release(temp_proc->proc_lock);
  }

  *num_ret = pid;
  *stat_loc = temp_proc -> s_exit;
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
  array_add(curproc->childprocs, childproc, &index_ret);
  KASSERT(array_get(curproc->childprocs, index_ret) == childproc);
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
  (void)exitcode;
  for(int i = 1; i < MAX_NUM_PROC; i++) {
    if(procs[i] != NULL){
      if(procs[i]->parent_pid == curproc->proc_pid){
        procs[i]->parent_pid = 0;
      }
    }
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
