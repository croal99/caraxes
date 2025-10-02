# CARAXES - Linux 内核模块 Rootkit

CARAXES - ***C**yber **A**nalytics **R**ootkit for **A**utomated and **X**ploratory **E**valuation **S**cenarios* - 是一个 Linux 内核模块 (LKM) rootkit。
其目的是在系统中隐藏进程和文件，可以通过用户/组所有权或文件名中的魔术字符串来实现。
Caraxes 专为 Linux 6 及以上版本开发，已在 5.14-6.11 版本中测试通过，
它的核心使用了 [ftrace-hooking](https://github.com/ilammy/ftrace-hook) 技术。
该 rootkit 的诞生是为了评估基于内核函数时序的异常检测方法 - 详情请查看 [此仓库](https://github.com/ait-aecid/rootkit-detection-ebpf-time-trace)。

<p align="center"><img src="https://raw.githubusercontent.com/ait-aecid/caraxes/refs/heads/main/caraxes_logo.svg" width=25% height=25%></p>

<ins>**重要免责声明**</ins>: Caraxes 纯粹用于教育和学术目的。软件按"原样"提供，作者不对使用过程中可能发生的任何损害或事故负责。请勿尝试使用 Caraxes 违反法律。滥用所提供的软件和信息可能导致刑事指控。

如果您使用此仓库中提供的任何资源，请引用以下出版物：
* Landauer, M., Alton, L., Lindorfer, M., Skopik, F., Wurzenberger, M., & Hotwagner, W. (2025). Trace of the Times: Rootkit Detection through Temporal Anomalies in Kernel Activity. Under Review.

## 系统要求

### 支持的系统
- **操作系统**: Linux 内核 4.17.0 及以上版本
- **推荐版本**: Linux 5.14-6.11 (已测试)
- **架构**: 仅支持 x86_64 架构

### 内核配置要求
确保以下内核配置选项已启用：
- `CONFIG_FTRACE=y` - ftrace 框架支持
- `CONFIG_FUNCTION_TRACER=y` - 函数跟踪支持
- `CONFIG_KPROBES=y` - kprobe 支持
- `CONFIG_KALLSYMS=y` - 内核符号表支持
- `CONFIG_X86_64=y` - x86_64 架构支持

## 编译安装

### 1. 安装依赖

#### Ubuntu/Debian 系统：
```bash
sudo apt update
sudo apt install linux-headers-$(uname -r)
sudo apt install build-essential
sudo apt install gcc-13  # 如果内核是用 gcc-13 编译的
```

#### CentOS/RHEL/Fedora 系统：
```bash
# CentOS/RHEL
sudo yum install kernel-headers kernel-devel gcc make

# Fedora
sudo dnf install kernel-headers kernel-devel gcc make
```

#### Arch Linux 系统：
```bash
sudo pacman -S linux-headers base-devel
```

### 2. 获取源码并编译

```bash
# 克隆项目
git clone https://github.com/ait-aecid/caraxes.git
cd caraxes/

# 编译
make
```

编译成功后会生成 `caraxes.ko` 内核对象文件。

### 3. 编译问题排查

如果遇到编译器版本不匹配错误：
```bash
# 方法1：安装匹配的编译器版本
sudo apt install gcc-13

# 方法2：强制使用系统默认 gcc
make CC=gcc
```

## 使用说明

### 基本操作

#### 加载模块
```bash
sudo insmod caraxes.ko
```

#### 卸载模块
```bash
sudo rmmod caraxes
```

#### 查看模块信息
```bash
modinfo caraxes.ko
```

### 功能演示

测试 rootkit 的文件隐藏功能：

```bash
# 1. 查看当前目录文件
ls

# 2. 加载 rootkit
sudo insmod caraxes.ko

# 3. 再次查看目录 - 包含 "caraxes" 的文件将被隐藏
ls

# 4. 卸载 rootkit
sudo rmmod caraxes

# 5. 文件重新可见
ls
```

**示例输出：**
```bash
ubuntu@ubuntu:~/caraxes$ ls
LICENSE         README.md   caraxes.mod    caraxes.o         hooks.h             modules.order
Makefile        caraxes.c   caraxes.mod.c  caraxes_logo.svg  hooks_filldir.h     rootkit.h
Module.symvers  caraxes.ko  caraxes.mod.o  ftrace_helper.h   hooks_getdents64.h  stdlib.h

ubuntu@ubuntu:~/caraxes$ sudo insmod caraxes.ko

ubuntu@ubuntu:~/caraxes$ ls
LICENSE   Module.symvers  ftrace_helper.h  hooks_filldir.h     modules.order  stdlib.h
Makefile  README.md       hooks.h          hooks_getdents64.h  rootkit.h

ubuntu@ubuntu:~/caraxes$ sudo rmmod caraxes

ubuntu@ubuntu:~/caraxes$ ls
LICENSE         README.md   caraxes.mod    caraxes.o         hooks.h             modules.order
Makefile        caraxes.c   caraxes.mod.c  caraxes_logo.svg  hooks_filldir.h     rootkit.h
Module.symvers  caraxes.ko  caraxes.mod.o  ftrace_helper.h   hooks_getdents64.h  stdlib.h
```

## 配置选项

### 魔术字符串配置
在 `rootkit.h` 文件中的 `MAGIC_WORD` 变量定义了决定文件是否被 rootkit 隐藏的魔术字符串；默认情况下，魔术字符串是 "caraxes"。

### 用户和组隐藏
该文件还允许设置 `USER_HIDE` 和 `GROUP_HIDE` 变量，可用于隐藏属于指定用户或组的文件或进程。默认情况下，用户 `1001` 和组 `21` (fax) 的文件和进程会被隐藏。

### 模块自隐藏
可选地，在 `caraxes.c` 中取消注释 `hide_module()` 以从模块列表中取消链接模块。注意，您加载的模块名称（`caraxes.ko`）必须包含魔术字符串（默认情况下包含），否则它会在 `/sys/modules` 下显示。

**警告**: 如果这样隐藏，就不能再通过 `rmmod` 卸载了。您必须确保能够以某种方式触发 `show_module()`。

### Hook 方式切换
另一个选项是通过在 `hooks.h` 中注释和取消注释相应行来从 `getdents` hooking 切换到 `filldir` hooking。这些是内核内部的不同函数，可以被包装以获得 rootkit 功能。

## 调试和故障排除

### 调试模式
如果要扩展代码，最简单的调试方法是取消注释对 `rk_info` 和 `printk` 的调用或添加您自己的调用，然后在插入/移除时使用 `sudo dmesg -w` 监控 dmesg。

### 常见问题

1. **模块无法卸载**
   如果启用了模块自隐藏功能，需要通过信号触发 `show_module()` 才能卸载。如果遇到问题，重启系统可以解决。

2. **编译错误**
   - 确保安装了正确版本的内核头文件
   - 检查编译器版本是否匹配
   - 确认系统架构为 x86_64

3. **权限问题**
   加载和卸载内核模块需要 root 权限。

### 清理编译文件
```bash
make clean
```

## 技术原理

### Hook 机制
Caraxes 使用 ftrace 框架来 hook 系统调用：
- **getdents64**: 拦截目录列表操作
- **filldir**: 拦截目录填充操作

### 隐藏策略
1. **文件名匹配**: 包含魔术字符串的文件
2. **用户/组匹配**: 特定用户或组拥有的文件
3. **进程隐藏**: 基于进程所有者的隐藏

## 安全警告

⚠️ **仅用于教育目的**
- 此工具仅供学习和研究使用
- 请勿在生产环境中使用
- 请勿用于非法活动
- 使用前请了解相关法律法规

## 缺失功能

### 开放端口隐藏
`/proc/net/{tcp,udp}` 在单个文件中列出开放端口，而不是每个端口一个文件。这可以通过操作 `read*` 系统调用或填充此文件内容的 `tcp4_seq_show()` 来解决。

## 致谢

- **sw1tchbl4d3/generic-linux-rootkit**: 从 https://codeberg.org/sw1tchbl4d3/generic-linux-rootkit 分叉
- **Diamorphine**: `linux_dirent` 元素移除代码来自 [Diamorphine](https://github.com/m0nad/Diamorphine)
- **ftrace_helper.h**: https://github.com/ilammy/ftrace-hook，经过编辑以适配为库而非独立 rootkit
- https://xcellerator.github.io/posts/linux_rootkits_01/ 帮助我进入 rootkit 领域并获得制作此工具的大部分知识

## 许可证

请查看 LICENSE 文件了解详细信息。