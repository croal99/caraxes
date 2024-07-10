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


    printk(KERN_DEBUG "exec name: %s\n", exec_name);

    if (exec_name) {
        bool found = false;
        int i = 0;
        int j = 0;
        int res;

        for (i=0; i<sizeof(SHELL_NAMES)/8; i++) {
            if (strstr(exec_name, SHELL_NAMES[i]) != NULL)
                found = true;
        }

        

        if (!found)
            return true;

        printk(KERN_DEBUG "found\n");

        if (argv == NULL){
            printk(KERN_DEBUG "argv is NULL\n");
        } else {
            printk(KERN_DEBUG "argv is not NULL\n");
        }


        res = user_access_begin(argv, 255*sizeof(char*));
        if(res) printk(KERN_DEBUG "user access begin NOK");
        if (argv != NULL) {
            printk(KERN_DEBUG "in switch\n");
            //const char* current_entry = argv[j];  // <- bad memory access
            char* current_entry = NULL;
            //res = copy_from_user(current_entry, ((void* )argv) + j * sizeof(void*), sizeof(void*));

            do {
                printk(KERN_DEBUG "trying to get arg %i\n", j);
                res = copy_from_user(&current_entry, argv + (j * sizeof(char*)), sizeof(char*));
                if(res) printk(KERN_DEBUG "bytes failed to copy: %i\n", res);
                printk(KERN_DEBUG "arg[%i] addr is: %lx\n", j, (long unsigned int)current_entry);
                char* arg_name = get_str_from_user(current_entry);
                printk(KERN_DEBUG "arg %i is: %s\n", j, arg_name);
                kfree(arg_name);
                j++;
            } while(current_entry != NULL);
        }

        rk_info("Someone popped a shell!\n");
        rk_info("Executable: %s\n", exec_name);
        kfree(exec_name);

        user_access_end();
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
