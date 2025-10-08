# 代码总览（Crystal 内核模块）

本文件概述项目的工作原理、核心机制与各源码文件的职责划分，帮助快速理解与扩展代码。

## 设计概览

- 模块类型：Linux 内核模块（LKM），用于隐藏文件与进程。
- Hook 技术：基于 ftrace 的符号解析与函数替换，当前默认拦截 `sys_getdents64`。
- 隐藏策略：
  - 文件名包含魔术字符串（`MAGIC_WORD`，默认值为 `"crystal"`）。
  - 目录项属主用户 ID（`USER_HIDE`）或组 ID（`GROUP_HIDE`）匹配时隐藏。
- 产物命名：编译生成 `crystal.ko`。

## 核心机制

- 目录枚举拦截（默认）：
  - `hooks.h` 中注册 `HOOK("sys_getdents64", hook_sys_getdents64, &orig_sys_getdents64)`。
  - `hooks_getdents64.h` 的 `hook_sys_getdents64(...)` 在调用原始系统调用后，调用 `evil(...)` 对返回的目录项缓冲区进行过滤。
  - `evil(...)` 会：
    - 遍历用户缓冲区中的 `linux_dirent`/`linux_dirent64`，调用 `vfs_fstatat` 查询每个条目的属主 `uid/gid`。
    - 若满足隐藏条件（魔术字符串/用户/组），通过调整 `reclen` 与 `memmove` 从缓冲区移除该条目。
    - 将过滤后的结果复制回用户空间。
- 可选的 `filldir` 拦截：
  - `hooks_filldir.h` 提供 `filldir/fillonedir/compat_filldir` 等变体的 Hook，仅基于 `MAGIC_WORD` 过滤。
  - 在 `hooks.h` 中默认注释，可按需启用测试不同 hook 策略。

## 文件说明

- `crystal.c`
- 模块入口与退出：`init_module`/`cleanup_module` 或 `module_init`/`module_exit`。
- 初始化并安装 ftrace 钩子，卸载时恢复原始函数。
- 可能包含辅助逻辑（如信号处理入口、调试输出）。
- `rootkit.h`
  - 配置项与通用工具：
    - `MAGIC_WORD`、`USER_HIDE`、`GROUP_HIDE` 用于控制隐藏策略。
    - 模块自隐藏：`hide_module()`/`show_module()` 操作 `THIS_MODULE->list` 链表以隐藏/恢复模块在 `/sys/modules` 的可见性。
    - 提权示例：`set_root()` 修改当前任务凭据为 `root`。
    - 进程信息聚合：`get_current_process()` 收集当前任务相关结构体指针。
- `hooks.h`
  - 统一注册处：通过宏 `HOOK`/`HOOK_NOSYS` 定义需要安装的钩子数组 `syscall_hooks[]`。
  - 默认启用 `sys_getdents64`，其他 `filldir` 相关钩子为注释状态。
- `hooks_getdents64.h`
  - 目录项过滤核心实现：`evil(...)` 按魔术字符串与属主 UID/GID 从用户缓冲区移除条目。
  - 钩子包装：`hook_sys_getdents64(...)` 负责调用原始系统调用并将结果交给 `evil(...)` 处理。
- `hooks_filldir.h`
  - 多版本 `filldir`/`fillonedir`/`compat_filldir` 的拦截实现，按 `MAGIC_WORD` 过滤条目。
  - 供与 `getdents64` 对比测试不同 Hook 点的行为与兼容性。
- `ftrace_helper.h`
  - ftrace 辅助框架：
    - 定义 `struct ftrace_hook`，包含目标符号名、替换函数指针、原函数指针保存等。
    - 负责解析内核符号地址（如 `lookup_name`）并安装/移除 ftrace 钩子，切换到自定义函数。
  - 为 `hooks.h` 的宏提供支撑（注册、启用、禁用）。
- `stdlib.h`
  - 内核态“标准库”工具：
    - 线程与进程相关：`stop_kthread`、`system_internal`/`execve`（基于 `call_usermodehelper`）。
    - 一些便捷包装用于演示或扩展。
- `Makefile`
  - Kbuild 构建文件：
    - `obj-m += crystal.o` 指定模块名；模块源文件为 `crystal.c`（单源构建）。
    - `make -C /lib/modules/$(uname -r)/build M=$(PWD) modules` 使用当前内核构建树。
- `build.sh`
  - 非 Docker 的主机编译脚本：
    - 支持 `-m` 菜单选择内核版本（如 `6.8.0-79-generic`），或自动使用 `uname -r`。
    - 校验 `/lib/modules/<KVER>/build` 是否存在；可清理并触发 `make` 构建。
    - 编译完成检查并提示 `crystal.ko` 路径。
- `scripts/docker-make.sh`、`scripts/docker-run.sh`
  - Docker 相关脚本：在容器环境中构建或运行，保证依赖可复现与隔离（需要在容器内提供匹配的内核头文件）。
- `src/caraxes.jsp`、`src/shell.jsp`
  - 与内核模块无直接耦合的示例/演示页面，源于评估与演示场景；可忽略于内核侧工作流。

## 数据流与调用关系

- 用户态发起目录枚举（例如 `ls` 或 `ps` 依赖的 `/proc` 枚举）。
- 内核命中 `sys_getdents64` → 被 ftrace 重定向到 `hook_sys_getdents64`。
- 执行原始 `getdents64` → 返回用户缓冲区长度与内容。
- 调用 `evil(...)`：遍历并删除需隐藏的目录项 → 复制回用户缓冲区。
- 用户态只看到被过滤后的结果，实现“不可见”。

## 配置与扩展

- 调整隐藏策略：修改 `rootkit.h` 中的 `MAGIC_WORD`、`USER_HIDE`、`GROUP_HIDE`。
- 切换 Hook 点：在 `hooks.h` 中启用 `filldir` 系列以测试不同版本/内核的兼容性与行为差异。
- 调试：在关键路径处启用 `printk` 或专用日志宏，使用 `dmesg -w` 观察加载/卸载与过滤行为。

## 编译与产物

- 直接构建：`make`（依赖 `/lib/modules/$(uname -r)/build` 链接到对应的内核构建树）。
- 指定版本：`./build.sh --version <KVER>` 或 `./build.sh -m` 菜单选择。
- 成功后产物：`crystal.ko` 位于项目根目录。

## 注意事项

- 版本/头文件匹配：确保内核版本与 `kernel-devel`/`headers` 一致（见 README_CN.md 的“编译问题排查”）。
- Secure Boot：加载未签名模块可能失败，需签名或禁用。
- 模块自隐藏：启用后可能无法 `rmmod` 卸载，需要能触发 `show_module()` 恢复。
