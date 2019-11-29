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
#include <cdefs.h>
#include <synch.h>
#include <spinlock.h>
#include <mips/tlb.h>
#include <vm.h>

paddr_t get_cppage(unsigned npages);

extern unsigned num_pages;
extern struct coremap my_coremap;
static struct spinlock cm_lock = SPINLOCK_INITIALIZER;
static unsigned initialized = 0;
paddr_t my_space;

void vm_bootstrap(void) {
  paddr_t all_space = ram_getsize();
  //The space that kernel has used
  paddr_t used_space = ram_getfirstfree();
  num_pages = all_space / PAGE_SIZE;
  unsigned used_pages = used_space / PAGE_SIZE;
  my_coremap.cm_entries = (struct entries*)PADDR_TO_KVADDR(used_space);
  my_space = used_space + ((all_space - used_space) / PAGE_SIZE) * sizeof(struct entries);
  my_space = ROUNDUP(my_space, PAGE_SIZE);

  //Initialize all entries:
  //Set the used entries as used
  for(unsigned i = 0; i < num_pages; i++) {
    if(i < used_pages) {
      my_coremap.cm_entries[i].used = 1;
    }
    else my_coremap.cm_entries[i].used = 0;
    my_coremap.cm_entries[i].first_page = 0;
    my_coremap.cm_entries[i].last_page = 0;
  }
  my_coremap.cm_entries[0].first_page = 1;
  my_coremap.cm_entries[num_pages-1].last_page = 1;
  initialized = 1;
}


struct v_entries1 {
  struct v_entries2* v_entries2;
};

struct v_entries2{
  struct v_entries3* v_entries3;
};
struct v_entries3{
  struct v_entries4* v_entries4;
};
struct v_entries4{
  paddr_t* padddr;
};

// unsigned vaddr1 = (unsigned)faultaddress >> 15;
// unsigned vaddr2 = ((unsigned)faultaddress >> 10) & 0x1F;
// unsigned vaddr3 = ((unsigned)faultaddress >> 5) & 0x1F;
// unsigned vaddr4 = (unsigned)faultaddress & 0x1F;
//
// if(v_entries1[vaddr1])->v_entries2 == NULL) {
//   v_entries1[vaddr1]->v_entries2 = kmalloc(sizeof(struct v_entries2));
// }
// if(v_entries2[vaddr2]->v_entries3 == NULL) {
//   v_entries2[vaddr2]->v_entries3 = kmalloc(sizeof(struct v_entries3));
// }
// if(v_entries3[vaddr3]->v_entries4 == NULL) {
//   v_entries3[vaddr3]->v_entries4 = kmalloc(sizeof(struct v_entries3));
// }
//Get consecutive physical pages
paddr_t get_cppage(unsigned npages) {
  spinlock_acquire(&cm_lock);
  unsigned k = 0;
  unsigned i;
  for(i = 0; i < num_pages && k < npages; i++) {
    if(!my_coremap.cm_entries[i].used) {
      k++;
    }
    else {
      k = 0;
    }
  }
  if(k != npages) {
    spinlock_release(&cm_lock);
    return 0;//Out of Memory: no enough consecutive pages in coremap
  }

  unsigned base_page = i - k;
  unsigned last_page = i - 1;

  my_coremap.cm_entries[base_page].first_page = 1;
  my_coremap.cm_entries[last_page].last_page = 1;
  for(unsigned m = base_page; m <= last_page; m++) {
    my_coremap.cm_entries[m].used = 1;
  }
  spinlock_release(&cm_lock);
  return PADDR_TO_KVADDR((paddr_t)(base_page << 12)); //TO RAM SHIFT
}
/* Fault handling function called by trap code */
int vm_fault(int faulttype, vaddr_t faultaddress){
  // switch (faulttype) {
  //   case VM_FAULT_READ: //TO DO
  //   case VM_FAULT_WRITE: // TO DO
  //   break;
  // }
  // //For kernel
  // if(curproc->proc_pid <= 1) {
  //   int spl = splhigh();
  //   unsigned v_pnum = ((unsigned)faultaddress >> 12);
  //   (void) spl;
  //   (void) v_pnum;
  // }
  // kprintf("FULLADDRESS: 0x%u\n", (unsigned)faultaddress);
  (void) faulttype;
  (void) faultaddress;
  return -1;
}



/* Allocate/free kernel heap pages (called by kmalloc/kfree) */
vaddr_t alloc_kpages(unsigned npages) {
  if(initialized == 0) {
    spinlock_acquire(&cm_lock);
    paddr_t pa = ram_stealmem(npages);
    // unsigned temp = (unsigned)pa;
    // KASSERT(temp != 0 && temp != 0x80000000);
    spinlock_release(&cm_lock);
    return PADDR_TO_KVADDR(pa);
  }
   unsigned temp = (unsigned)get_cppage(npages);
   // KASSERT(temp != 0 && temp != 0x80000000);
   return (vaddr_t)temp;
}

void free_kpages(vaddr_t addr) {
  spinlock_acquire(&cm_lock);
  paddr_t paddr = addr & 0x7FFFFFFF;
  unsigned i = (unsigned)(paddr >> 12);
  my_coremap.cm_entries[i].first_page = 0;
  while(!my_coremap.cm_entries[i].last_page) {
      my_coremap.cm_entries[i].used = 0;
      i++;
  }
  my_coremap.cm_entries[i].last_page = 0;
  my_coremap.cm_entries[i].used = 0;
  spinlock_release(&cm_lock);
}

/* TLB shootdown handling called from interprocessor_interrupt */
void vm_tlbshootdown_all(void) {
  panic("TLB shootdown unsupported!!!\n");
}
void vm_tlbshootdown(const struct tlbshootdown * temp) {
  (void)temp;
  panic("TLB shootdown unsupported!!!\n");
}
