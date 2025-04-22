# CARAXES - Linux Kernel Module Rootkit

CARAXES - ***C**yber **A**nalytics **R**ootkit for **A**utomated and **X**ploratory **E**valuation **S**cenarios* - is a Linux Kernel Module (LKM) rootkit.
The purpose is to hide processes and files on a system, this can be done via user/group ownership or a magic-string in the filename.
Caraxes was developted for Linux versions 6 and up, and has been tested for 5.14-6.11,
it uses [ftrace-hooking](https://github.com/ilammy/ftrace-hook) at its core.
The rootkit was born to evaluate anomaly detection approaches based on kernel function timings - check out [this repository](https://github.com/ait-aecid/rootkit-detection-ebpf-time-trace) for details.

<p align="center"><img src="https://raw.githubusercontent.com/ait-aecid/caraxes/refs/heads/main/caraxes_logo.svg" width=25% height=25%></p>

<ins>__Important Disclaimer__</ins>: Caraxes is purely for educational and academic purposes. The software is provided "as is" and the authors are not responsible for any damage or mishaps that may occur during its use. Do not attempt to use Caraxes to violate the law. Misuse of the provided software and information may result in criminal charges.

If you use any of the resources provided in this repository, please cite the following publication:
* Landauer, M., Alton, L., Lindorfer, M., Skopik, F., Wurzenberger, M., & Hotwagner, W. (2025). Trace of the Times: Rootkit Detection through Temporal Anomalies in Kernel Activity. Under Review.

## Compilation

Install the kernel headers (`apt install linux-headers-$(uname -r)` / `yum install kernel-headers` / `pacman -S linux-headers`).

```sh
$ git clone https://github.com/ait-aecid/caraxes.git
$ cd caraxes/
$ make
```

This gives you the `caraxes.ko` koernelobject file, which can be loaded via `insmod caraxes.ko`.
Remove it via `rmmod caraxes` , given it is not hidden (see `hide_module()`).

### Try it out

If you loaded the rootkit on your local system in your current working directory, try listing the files - many of them should be gone,
because the standard magic-string for files to hide is "caraxes".

## Configuration

The magic word that determines whether a file is hidden by the rootkit or not is defined in variable `MAGIC_WORD` in the file `rootkit.h`; by default, the magic word is "caraxes". This file also allows to set the variables `USER_HIDE` and `GROUP_HIDE`, which can be used to hide files or processes that belong to the specified user or group. By default, files and processes of user `1001` and group `21` (fax) are hidden.

Optionally, uncomment the `hide_module()` in `caraxes.c` to unlink the module from the modules list. Note that the name of the module that you load (`caraxes.ko`) has to contain the magic word (it does by default), otherwise it will show up under `/sys/modules`.
If it is hidden like this, it can not be unloaded via `rmmod` anymore.
You have to make sure to be able to trigger a `show_module()` [somehow](https://codeberg.org/sw1tchbl4d3/generic-linux-rootkit/src/branch/main/examples).

Another option is to switch from `getdents` hooking to `filldir` hooking by commenting and uncommenting the respective lines in `hooks.h`.
Those are different functions inside the kernel, that can be wrapped to get rootkit functionality.
We implemented different versions to test our [rootkit detection](https://github.com/ait-aecid/rootkit-detection-ebpf-time-trace).

## Troubleshooting

Keep in mind that if you unlink the module from the modules list (uncommenting of `hide_module()`), then `rmmod` will not find it and you will have to somehow signal to the rootkit to unhide itself with `show_module()`. If you get into that situation and the unhide does not work, or the kernel module crashed on `rmmod` or similar, a system restart should always do the trick.

If you want to extend the code, the easiest way is to debug the code is to uncomment the calls to `rk_info` and `printk` or add your own, then monitor dmesg on insert / remove with `sudo dmesg -w`.

## Missing Features: Open Ports

`/proc/net/{tcp,udp}` list open ports in a single file instead of one by port.
This can be addressed either by mangling with the `read*` syscalls or `tcp4_seq_show()`, which fills the content of this file.
Additionally, `/sys/class/net` shows statistics of network activity, which could hint to an open port.
Also `getsockopt` would fail when trying to bind to an open port - we would kind of have to flee, give up our port,
and start using a different one.

## Credits
- **sw1tchbl4d3/generic-linux-rootkit**: forked from https://codeberg.org/sw1tchbl4d3/generic-linux-rootkit
- **Diamorphine**: `linux_dirent` element removal code from [Diamorphine](https://github.com/m0nad/Diamorphine)
- `ftrace_helper.h`: https://github.com/ilammy/ftrace-hook, edited to fit as a library instead of a standalone rootkit.
- https://xcellerator.github.io/posts/linux_rootkits_01/, got me into rootkits and helped me gain most of the knowledge to make this. Much of the code is inspired by the code found here.
