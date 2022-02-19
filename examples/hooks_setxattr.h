// An example I made when I accidentally corrupted my HDD with a qemu image on it,
// which refused to boot due to setxattr failing.
// Very good usecase for a rootkit, I know.

#pragma once

#include <linux/string.h>
#include <linux/linkage.h>
#include <linux/stddef.h>

#include "stdlib.h"
#include "ftrace_helper.h"

bool do_setxattr(const char* path) {
    char* kernel_path = get_str_from_user(path);

    if (kernel_path) {
        if (strstr(kernel_path, "/media/veracrypt1") != NULL) {
            rk_info("IGNORE: %s\n", kernel_path);
            kfree(kernel_path);
            return false;
        }
        rk_info("PASS: %s\n", kernel_path);
    }

    kfree(kernel_path);
    return true;
}

#ifdef PTREGS_SYSCALL_STUBS
static asmlinkage long (*orig_setxattr)(const struct pt_regs*);

asmlinkage int hook_setxattr(const struct pt_regs* regs) {
    const char __user* path = FIRST_ARG(regs, const char*);
    
    if (do_setxattr(path))
        return orig_setxattr(regs);
    return 0;
}
#else
static asmlinkage long (*orig_setxattr)(const char __user* path, const char __user* name, const void __user* value, size_t size, int flags);

asmlinkage int hook_setxattr(const char __user* path, const char __user* name, const void __user* value, size_t size, int flags) {
    if (do_setxattr(path))
        return orig_setxattr(path, name, value, size, flags);
    return 0;
}
#endif

static struct ftrace_hook syscall_hooks[] = {
    HOOK("sys_setxattr",  hook_setxattr,  &orig_setxattr),
};
