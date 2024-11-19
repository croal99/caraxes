# Caraxes

Linux kernel module rootkit for Linux versions 6 and up, tested until Linux 6.11.

## Features

Hiding of files and processes by name or owner.

## Configuration

Define the magic string a file has to contain to be hidden in `rootkit.h`:`MAGIC_WORD`, default is "caraxes".
Also define the `USER_HIDE` and `GROUP_HIDE` there, 1001 and 21 (fax) are the defaults respectively,
if a file/process belongs to either it will be hidden.

### Module Hiding

Optionally uncomment the `hide_module()` in `caraxes.c`, this will unlink the module from the modules list.
Also the name of the module that you load (`caraxes.ko`) has to contain the magic word (it does by default),
otherwise it will show up under `/sys/modules`.

### Hook Target

You can optionally switch from `getdents` hooking to `filldir` hooking by commenting and uncommenting the respective lines in `hooks.h`.

## Installing

First, make sure you have installed the header libraries for your kernel.
On debian-like systems: `apt-get install linux-headers-$(uname -r)`
On arch-like systems: `pacman -S linux-headers` / `pacman -S linux-zen-headers` etc

```sh
make
sudo insmod caraxes.ko

# To remove
sudo rmmod caraxes
make clean
```

Keep in mind that if you hid the rootkit with `hide_module()` that `rmmod` will not find it, you'll have to somehow signal to the rootkit to unhide itself with `show_module()`.

If you somehow get into that situation, and the unhide doesn't work, or the kernel module crashed on `rmmod`or similar, a restart should do the trick.

## Debugging

The easiest way is to debug is to uncomment the calls to `rk_info` and `printk` or add your own,
then monitor dmesg on insert / remove with `sudo dmesg -w`.

## Missing Features

#### Open Ports

`/proc/net/{tcp,udp}` list open ports in a single file instead of one by port.
This can be addressed either by mangling with the `read*` syscalls or with `tcp4_seq_show()` which fills the content of this file.
Additionally `/sys/class/net` shows statistics of network activity, which could hint to an open port.
Also `getsockopt` would fail when trying to bind to an open port - we would kind of have to flee, give up our port,
and start using a different one.

## Credits
- **sw1tchbl4d3/generic-linux-rootkit**: forked from https://codeberg.org/sw1tchbl4d3/generic-linux-rootkit
- **Diamorphine**: `linux_dirent` element removal code from [Diamorphine](https://github.com/m0nad/Diamorphine)
- `ftrace_helper.h`: https://github.com/ilammy/ftrace-hook, edited to fit as a library instead of a standalone rootkit.
- https://xcellerator.github.io/posts/linux_rootkits_01/, got me into rootkits and helped me gain most of the knowledge to make this. Much of the code is inspired by the code found here.
