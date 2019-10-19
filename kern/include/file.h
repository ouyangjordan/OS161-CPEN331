
#ifndef _FILE_H_
#define _FILE_H_

#include <kern/limits.h>
#include <kern/unistd.h>
#include <types.h>
#include <vnode.h>


struct file {
    struct lock* file_lock;
    int file_flag;
    struct vnode* file_vnode;
    off_t file_offset;
    int count;
};

struct file* file_create(void);
void file_cleanup(struct file * filetable);
#endif
