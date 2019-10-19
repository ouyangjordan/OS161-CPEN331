//#include <kern/limits.h>
#include <kern/unistd.h>
#include <file.h>
#include <thread.h>
#include <synch.h>
#include <vnode.h>


struct file* file_create(void){
    struct file* filetable;
    filetable = kmalloc(sizeof(struct file));

    filetable->file_lock = lock_create("filelock");
    filetable->file_flag = 0;
    filetable->file_vnode = NULL;
    filetable->file_offset = 0;
    filetable->count = 0;//ref_count

    return filetable;

}

void file_cleanup(struct file* filetable){
    lock_destroy(filetable->file_lock);
    kfree(filetable);
}
