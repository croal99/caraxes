# generic-linux-rootkit

This is supposed to be a simple template rootkit, which can easily be built upon.

It allows for syscall hooking with ftrace, has hiding functionality, and has many kernel functions translated to more userspace-like C functions in `src/stdlib.h`.

## Installing

First, make sure you have installed the header libraries for your kernel.

On debian-like systems: `apt-get install linux-headers-$(uname -r)`

On arch-like systems: `pacman -S linux-headers` / `pacman -S linux-zen-headers` etc

After you've edited some of the files, or copied one of the examples (e.g. `cp examples/hooks_kill_backdoor.h src/hooks.h`), you can make and insert the rootkit like so:

```sh
cd src
make
sudo insmod rk.ko

# To remove
sudo rmmod rk
make clean
```

Keep in mind that if you hid the rootkit with `hide_module()` that rmmod will not find it, you'll have to somehow signal to the rootkit to unhide itself with `show_module()`.

If you somehow get into that situation, and the unhide doesnt work, or the kernel module crashed on rmmod or similar, a restart should do the trick.

A tip of mine is to always monitor dmesg on insert / remove with `dmesg -w`.

## Credits
- **sw1tchbl4d3/generic-linux-rootkit**: forked from https://codeberg.org/sw1tchbl4d3/generic-linux-rootkit
- **Diamorphine**: this is kind of a frankenstein rootkit made from GLRK and [Diamorphine](https://github.com/m0nad/Diamorphine)
- `ftrace_helper.h`: https://github.com/ilammy/ftrace-hook, edited to fit as a library instead of a standalone rootkit.
- https://xcellerator.github.io/posts/linux_rootkits_01/, got me into rootkits and helped me gain most of the knowledge to make this. Much of the code is inspired by the code found here.
