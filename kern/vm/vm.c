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
void print_coremap(void);
void check_coremap(void);
void panic_coremap(void);

extern unsigned num_pages;
extern struct coremap my_coremap;
static struct spinlock cm_lock = SPINLOCK_INITIALIZER;
static unsigned initialized = 0;
paddr_t my_space;

//Function to print the coremap
void print_coremap(void) {
  for(unsigned i = 0; i < num_pages; i++) {
    struct entries e = my_coremap.cm_entries[i];
    kprintf("entry: 0x%x  used: %d  first: %d  last: %d\n", i, e.used, e.first_page, e.last_page);
  }
}

//Function to check the coremap and see if the
//first or last entries are not being used
void check_coremap(void) {
  for(unsigned i = 0; i < num_pages; i++) {
    struct entries e = my_coremap.cm_entries[i];
    if (e.used == 0 && (e.first_page == 1 || e.last_page == 1)) {
      panic_coremap();
    }
  }
}

//Panic if coremap is not behaving properly
void panic_coremap() {
  panic("panic coremap");
}

//Function called on boot of sys161 os161
void vm_bootstrap(void) {

  paddr_t all_space = ram_getsize();
  //The space that kernel has used
  paddr_t used_space = ram_getfirstfree();
  num_pages = all_space / PAGE_SIZE;
  unsigned used_pages = used_space / PAGE_SIZE;

  //Manaully created space for my_coremap
  my_coremap.cm_entries = (struct entries*)PADDR_TO_KVADDR(used_space);
  my_space = used_space + ((all_space - used_space) / PAGE_SIZE) * sizeof(struct entries);
  my_space = ROUNDUP(my_space, PAGE_SIZE);

  //Initialize all entries:
  //Set the used entries as used.
  for(unsigned i = 0; i < num_pages; i++) {
    if(i <= used_pages) {
      my_coremap.cm_entries[i].used = 1;
    }
    else my_coremap.cm_entries[i].used = 0;
    my_coremap.cm_entries[i].first_page = 0;
    my_coremap.cm_entries[i].last_page = 0;
  }

  //our TLB is initialized
  initialized = 1;

}


//Get consecutive physical pages
paddr_t get_cppage(unsigned npages) {
  
   
  unsigned k = 0;
  unsigned i;
  
  //Find if there is npages consecutive physical pages.
  //k is the count number that the number of consecutive
  //pages we found so far.
  for(i = 0; i < num_pages && k < npages; i++) {
    if(!my_coremap.cm_entries[i].used) {
      k++;
    }
    else {
      k = 0;
    }
  }

  //If npages consecutive physical pages are not found,
  //return 0.
  if(k != npages) {
    return 0;//Out of Memory: no enough consecutive pages in coremap
  }

  //Index of the first page and the last page
  unsigned base_page = i - k;
  unsigned last_page = i - 1;
  
  //Set the flags for both first page and last page
  my_coremap.cm_entries[base_page].first_page = 1;
  my_coremap.cm_entries[last_page].last_page = 1;

  //Set all pages between the first page and the last page
  //(included) as used.
  for(unsigned m = base_page; m <= last_page; m++) {
    my_coremap.cm_entries[m].used = 1;
  }
  //Transfer the physical address to virtual address
  return PADDR_TO_KVADDR((paddr_t)(base_page << 12)); //TO RAM SHIFT
}

/* Fault handling function called by trap code */
int vm_fault(int faulttype, vaddr_t faultaddress){
  (void) faulttype;
  (void) faultaddress;
  return -1;
}



/* Allocate/free kernel heap pages (called by kmalloc/kfree) */
vaddr_t alloc_kpages(unsigned npages) {

  spinlock_acquire(&cm_lock);
  

  if(initialized == 0) {
    paddr_t pa = ram_stealmem(npages);
  
    spinlock_release(&cm_lock);
    return PADDR_TO_KVADDR(pa);
  }
   
   unsigned temp = (unsigned)get_cppage(npages);
   
   spinlock_release(&cm_lock);
   return (vaddr_t)temp;
}

void free_kpages(vaddr_t addr) {
  
  spinlock_acquire(&cm_lock);

  paddr_t paddr = addr & 0x7FFFFFFF;
  unsigned i = (unsigned)(paddr >> 12);

  my_coremap.cm_entries[i].first_page = 0;
  while(!my_coremap.cm_entries[i].last_page) {
      my_coremap.cm_entries[i].first_page = 0;
      my_coremap.cm_entries[i].used = 0;
      my_coremap.cm_entries[i].last_page = 0;
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
