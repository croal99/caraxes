// Q&D example of how to potentially log / block system("/bin/sh") (or similar) calls.
// Again, just for testing, this isn't a real security measure.

#pragma once

#include <linux/string.h>
#include <linux/linkage.h>
#include <linux/stddef.h>

#include "stdlib.h"
#include "ftrace_helper.h"

const char* SHELL_NAMES[] = {"/bin/sh", "/bin/bash", "/bin/dash", "/bin/ksh", "/bin/zsh"};
const char* ARG_NAMES[] = {"-i", "-p", "-ip", "-pi", "-c", "-pic", "-ipc", "-cip", "-icp", "-pci", "-cpi"};

bool do_execve(const char *path, const char** argv) {
    char* exec_name = get_str_from_user(path);
    if (exec_name) {
        bool found = false;
        int i = 0;
        int j = 0;

        for (i=0; i<sizeof(SHELL_NAMES)/8; i++) {
            if (strstr(exec_name, SHELL_NAMES[i]) != NULL)
                found = true;
        }

        kfree(exec_name);

        if (!found)
            return true;

        if (argv != NULL) {
            const char* current_entry = argv[j];
            while (current_entry != NULL) {
                char* arg_name = get_str_from_user(current_entry);
                found = false;
                if (arg_name) {

                    for (i=0; i<sizeof(SHELL_NAMES)/8; i++) {
                        if (strstr(arg_name, SHELL_NAMES[i]) != NULL)
                            found = true;
                    }

                    for (i=0; i<sizeof(ARG_NAMES)/8; i++) {
                        if (strstr(arg_name, ARG_NAMES[i]) != NULL)
                            found = true;
                    }

                    kfree(arg_name);

                    if (!found)
                        return true;

                }
                current_entry = argv[++j];
            }
        }

        rk_info("Someone popped a shell!\n");
        rk_info("Executable: %s\n", exec_name);

        int x = 0;
        const char* current_entry = argv[x];

        while (current_entry != NULL) {
            char* arg = get_str_from_user(current_entry);
            if (arg) {
                rk_info("%d: %s\n", x, current_entry);
                kfree(arg);
            }
            current_entry = argv[++x];
        }
    }
    return true;
}

#ifdef PTREGS_SYSCALL_STUBS
static asmlinkage long (*orig_execve)(const struct pt_regs*);
static asmlinkage long (*orig_execveat)(const struct pt_regs*);

asmlinkage int hook_execve(const struct pt_regs* regs) {
    const char __user* path = FIRST_ARG(regs, const char*);
    const char __user** argv = SECOND_ARG(regs, const char**);
    
    if (do_execve(path, argv))
        return orig_execve(regs);
    return 0;
}

asmlinkage int hook_execveat(const struct pt_regs* regs) {
    const char __user* path = SECOND_ARG(regs, const char*);
    const char __user** argv = THIRD_ARG(regs, const char**);
    
    if (do_execve(path, argv))
        return orig_execveat(regs);
    return 0;
}
#else
static asmlinkage long (*orig_execve)(const char __user* filename, const char __user** argv, const char __user** envp);
static asmlinkage long (*orig_execveat)(int dfd, const char __user* filename, const char __user** argv, const char __user** envp, int flags);


asmlinkage int hook_execve(const char __user* filename, const char __user** argv, const char __user** envp) {
    if (do_execve(filename, argv))
        return orig_execve(filename, argv, envp);
    return 0;
}

asmlinkage int hook_execveat(int dfd, const char __user* filename, const char __user** argv, const char __user** envp, int flags) {
    if (do_execve(filename, argv))
        return orig_execveat(dfd, filename, argv, envp, flags);
    return 0;
}
#endif

static struct ftrace_hook syscall_hooks[] = {
    HOOK("sys_execve",  hook_execve,  &orig_execve),
    HOOK("sys_execveat",  hook_execveat,  &orig_execveat),
};
