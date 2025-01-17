# Caraxes

This repository contains the Linux kernel module (LKM) rootkit CARAXES, which stands for Cyber Analytics Rootkit for Automated and eXploratory Evaluation Scenarios. The rootkit can be used to hide files, directories, and processes. The rootkit is designed for Linux kernel versions of 6 and above, and was tested until version 6.11.

<p align="center"><img src="https://raw.githubusercontent.com/ait-aecid/caraxes/refs/heads/main/caraxes_logo.svg" width=25% height=25%></p>

<ins>__Important Disclaimer__</ins>: Caraxes is purely for educational and academic purposes. The software is provided "as is" and the authors are not responsible for any damage or mishaps that may occur during its use. Do not attempt to use Caraxes to violate the law. Misuse of the provided software and information may result in criminal charges.

If you use any of the resources provided in this repository, please cite the following publication:
* Landauer, M., Alton, L., Lindorfer, M., Skopik, F., Wurzenberger, M., & Hotwagner, W. (2025). Trace of the Times: Rootkit Detection through Temporal Anomalies in Kernel Activity. Under Review.

## Installation

First, install the following dependencies. Make sure to install the correct header libraries for your kernel: `linux-headers-$(uname -r)` should work on debian-like systems (see code below). On arch-like systems try `pacman -S linux-headers` / `pacman -S linux-zen-headers` instead.

```sh
ubuntu@ubuntu:~$ sudo apt update
ubuntu@ubuntu:~$ sudo apt install python3-bpfcc make gcc flex bison linux-headers-$(uname -r)
```

Second, download and compile the rootkit from this repository.

```sh
ubuntu@ubuntu:~$ git clone https://github.com/ait-aecid/caraxes.git
ubuntu@ubuntu:~$ cd caraxes/
ubuntu@ubuntu:~/caraxes$ sudo make
```

To test the rootkit, try to run `ls` in the directory - you should see several files as depicted below. Run `sudo insmod caraxes.ko` to load the rootkit into the kernel. Now, run `ls` again - all files that contain the magic word "caraxes" are hidden from the user. To make the files visible, just remove the rootkit from the kernel using `sudo rmmod caraxes`.

```sh
ubuntu@ubuntu:~/caraxes$ ls
LICENSE  Makefile  Module.symvers  README.md  caraxes.c  caraxes.ko  caraxes.mod  caraxes.mod.c  caraxes.mod.o  caraxes.o  caraxes_logo.svg  ftrace_helper.h  hooks.h  hooks_filldir.h  hooks_getdents64.h  modules.order  rootkit.h  stdlib.h
ubuntu@ubuntu:~/caraxes$ sudo insmod caraxes.ko
ubuntu@ubuntu:~/caraxes$ ls
LICENSE  Makefile  Module.symvers  README.md  ftrace_helper.h  hooks.h  hooks_filldir.h  hooks_getdents64.h  modules.order  rootkit.h  stdlib.h
ubuntu@ubuntu:~/caraxes$ sudo rmmod caraxes
ubuntu@ubuntu:~/caraxes$ ls
LICENSE  Makefile  Module.symvers  README.md  caraxes.c  caraxes.ko  caraxes.mod  caraxes.mod.c  caraxes.mod.o  caraxes.o  caraxes_logo.svg  ftrace_helper.h  hooks.h  hooks_filldir.h  hooks_getdents64.h  modules.order  rootkit.h  stdlib.h
ubuntu@ubuntu:~/caraxes$ make clean
```

## Configuration

The magic word that determines whether a file is hidden by the rootkit or not is defined in variable `MAGIC_WORD` in the file `rootkit.h`; by default, the magic word is "caraxes". This file also allows to set the variables `USER_HIDE` and `GROUP_HIDE`, which can be used to hide files or processes that belong to the specified user or group. By default, files and processes of user `1001` and group `21` (fax) are hidden.

Optionally, uncomment the `hide_module()` in `caraxes.c` to unlink the module from the modules list. Note that the name of the module that you load (`caraxes.ko`) has to contain the magic word (it does by default), otherwise it will show up under `/sys/modules`.

Another option is to switch from `getdents` hooking to `filldir` hooking by commenting and uncommenting the respective lines in `hooks.h`.

## Troubleshooting

Keep in mind that if you unlink the module from the modules list (uncommenting of `hide_module()`), then `rmmod` will not find it and you will have to somehow signal to the rootkit to unhide itself with `show_module()`. If you get into that situation and the unhide does not work, or the kernel module crashed on `rmmod`or similar, a system restart should always do the trick.

In case that you run into problems when installing the kernel header libraries, try to uninstall all headers and only install the ones for your kernel.

```sh
ubuntu@ubuntu:~/caraxes$ sudo apt remove linux-headers-*
ubuntu@ubuntu:~/caraxes$ sudo apt install linux-headers-$(uname -r)
```

If you want to extend the code, the easiest way is to debug the code is to uncomment the calls to `rk_info` and `printk` or add your own, then monitor dmesg on insert / remove with `sudo dmesg -w`.

## Missing Features: Open Ports

`/proc/net/{tcp,udp}` list open ports in a single file instead of one by port.
This can be addressed either by mangling with the `read*` syscalls or `tcp4_seq_show()`, which fills the content of this file.
Additionally `/sys/class/net` shows statistics of network activity, which could hint to an open port.
Also `getsockopt` would fail when trying to bind to an open port - we would kind of have to flee, give up our port,
and start using a different one.

## Disclaimer

Caraxes is purely for educational and academic purposes. The software is provided "as is" and the authors are not responsible for any damage or mishaps that may occur during its use.

Do not attempt to use Caraxes to violate the law. Misuse of the provided software and information may result in criminal charges.

## Credits
- **sw1tchbl4d3/generic-linux-rootkit**: forked from https://codeberg.org/sw1tchbl4d3/generic-linux-rootkit
- **Diamorphine**: `linux_dirent` element removal code from [Diamorphine](https://github.com/m0nad/Diamorphine)
- `ftrace_helper.h`: https://github.com/ilammy/ftrace-hook, edited to fit as a library instead of a standalone rootkit.
- https://xcellerator.github.io/posts/linux_rootkits_01/, got me into rootkits and helped me gain most of the knowledge to make this. Much of the code is inspired by the code found here.

# Citation

If you use any of the resources provided in this repository, please cite the following publication:
* Landauer, M., Alton, L., Lindorfer, M., Skopik, F., Wurzenberger, M., & Hotwagner, W. (2025). Trace of the Times: Rootkit Detection through Temporal Anomalies in Kernel Activity. Under Review.
