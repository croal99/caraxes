// Example of a TCP server in the kernel to test out
// the socket implementation from the stdlib.

#include <linux/module.h>
#include <linux/kernel.h>

#define DEBUG 1

#include "stdlib.h"
#include "ftrace_helper.h"
#include "hooks.h"

struct task_struct* task = NULL;
bool task_running = true;

MODULE_LICENSE("GPL");

int net_thread(void) {
    int rc;
    struct sockaddr_in sockaddr;
    char buf[50];
    struct socket* sock;
    struct socket* client_sock;

    char* msg = "Hello from the kernel!\n";
    task_running = true;

    rk_info("Entered net_thread!\n");

    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (!sock) {
        rk_info("Socket creation failed :( (%ld)\n", errno);
        task_running = false;
        return 1;
    }

    sockaddr.sin_addr.s_addr=htonl(INADDR_ANY);
    sockaddr.sin_family=AF_INET;
    sockaddr.sin_port=htons(4949);
    
    rc = bind(sock, (struct sockaddr*) &sockaddr, sizeof(sockaddr));

    if (rc < 0) {
        rk_info("Socket bind failed :( (%ld)\n", errno);
        release(sock);
        task_running = false;
        return 1;
    }

    rc = listen(sock, 50);
    if (rc < 0) {
        rk_info("Socket listen failed :( (%ld)\n", errno);
        release(sock);
        task_running = false;
        return 1;
    }

    rk_info("Waiting for client...\n");
    while (!kthread_should_stop()) {
        sleep(1000);

        client_sock = accept(sock, O_NONBLOCK);
        if (!client_sock) {
            if (errno == -EAGAIN)
                continue;

            rk_info("Socket accept failed :( (%ld)\n", errno);
            release(sock);
            task_running = false;
            return 1;
        }

        rk_info("Got a client!\n");

        rc = recvmsg(client_sock, buf, sizeof(buf));
        if (rc < 0) {
            rk_info("Socket read failed :( (%ld)\n", errno);
            task_running = false;
            return 1;
        }

        buf[sizeof(buf)-1] = '\0';
        rk_info("Someone sent us: %s\n", buf);

        rc = sendmsg(client_sock, msg, strlen(msg));
        if (rc < 0) {
            rk_info("Socket write failed :( (%ld)\n", errno);
            task_running = false;
            return 1;
        }

        release(client_sock);
    }

    release(sock);
    rk_info("Exiting net_thread...\n");

    task_running = false;
    return 0;
}

static int rk_init(void) {
    int err;

    err = fh_install_hooks(syscall_hooks, ARRAY_SIZE(syscall_hooks));
    if (err)
        return err;

    rk_info("module loaded\n");

    task = start_kthread((void*)net_thread);

    return 0;
}

static void rk_exit(void) {
    fh_remove_hooks(syscall_hooks, ARRAY_SIZE(syscall_hooks));

    if (task && task_running)
        stop_kthread(task);

    rk_info("module unloaded\n");
}

module_init(rk_init);
module_exit(rk_exit);
