# Crystal 内核模块

Crystal是一个用于演示与研究的 Linux 内核模块（LKM）rootkit，核心能力是在用户空间中隐藏文件与进程目录项。模块基于 ftrace 安装系统调用钩子，默认拦截 `sys_getdents64`，并支持在运行时通过 `/proc/crystal` 动态管理按 PID 隐藏进程。

免责声明：仅用于教育与研究目的。请勿在生产环境或违反法律的场景中使用。本仓库的作者不对使用造成的任何后果负责。

## 代码逻辑

- Hook 机制（ftrace）

  - 在 `hooks.h` 中注册：`HOOK("sys_getdents64", hook_sys_getdents64, &orig_sys_getdents64)`。
  - `hooks_getdents64.h` 中的 `hook_sys_getdents64(...)` 调用原始系统调用后，进入 `evil(...)` 对返回的目录项缓冲区进行过滤。
- 目录项过滤策略（evil）

  - 隐藏包含魔术字符串的文件名：`MAGIC_WORD`（默认 `"crystal"`，位于 `rootkit.h`）。
  - 隐藏特定属主：`USER_HIDE`（默认 `1001`）或组：`GROUP_HIDE`（默认 `21`）。
  - 隐藏进程目录：当枚举 `/proc` 时，若目录名为数字且该 PID 在隐藏列表中，则过滤该目录条目。
- 动态隐藏 PID（crystal.c）

  - 维护隐藏 PID 列表（链表 + `spinlock` 并发控制）：`hide_pid`、`unhide_pid`、`clear_hidden_pids`、`pid_is_hidden`。
  - 提供 `/proc/crystal` 接口：可读写。写入支持命令式管理；读取直接返回当前隐藏 PID 列表。
  - 仅在枚举 `/proc` 时按 PID 过滤，对其他文件系统不受影响（通过 `fd_is_proc` 检测）。
- 可选 Hook 点

  - `hooks_filldir.h` 提供 `filldir` 系列替代钩子，可在 `hooks.h` 中切换以测试不同内核版本兼容性与行为差异。

## 使用 `/proc/crystal`

- 查看当前隐藏的 PID 列表：`cat /proc/crystal`
- 添加隐藏：`echo "add 106808" | sudo tee /proc/crystal` 或直接写数字 `echo "106808" | sudo tee /proc/crystal`
- 删除隐藏：`echo "del 106808" | sudo tee /proc/crystal`
- 清空列表：`echo "clear" | sudo tee /proc/crystal`
- 打印列表到内核日志：`echo "list" | sudo tee /proc/crystal` 后用 `dmesg | tail -n 50` 查看

说明：`/proc/crystal` 权限为 `0644`，可直接 `cat` 输出隐藏 PID 列表。`list` 命令只写日志，不返回到终端标准输出。

## 编译（Makefile 与 build.sh）

- 直接编译

  - 依赖内核头文件与构建树：`/lib/modules/$(uname -r)/build`
  - 执行：`make`
  - 产物：`crystal.ko`
- 使用脚本 `build.sh`

  - 作用：为指定或当前内核版本准备依赖、修复构建链接、编译并复制产物到 `output/<KVER>/crystal.ko`。
  - 参数：
    - `-v, --version <KVER>` 指定目标内核版本（如 `5.14.0-570.35.1.el9_6.x86_64` 或 `6.8.0-79-generic`）
    - `-m, --menu` 打开菜单（当前主机内核 / 手动输入 / 退出）
    - `--no-clean` 跳过 `make clean`，直接编译
    - `-h, --help` 显示帮助
  - 示例：
    - 使用当前内核：`./build.sh`
    - 指定版本：`./build.sh --version 5.14.0-570.35.1.el9_6.x86_64`
    - 打开菜单选择：`./build.sh -m`
  - 说明：脚本会在缺少头文件或构建链接时尝试安装 `kernel-devel`/`linux-headers` 并修复 `/lib/modules/<KVER>/build` 指向。

## 安装与加载（ins.sh）

- 脚本 `ins.sh` 负责安装已编译好的模块到当前内核，并设置开机自动加载，默认立即加载。

  - 注意：不负责编译，请先确保当前目录存在 `crystal.ko`。
- 参数：

  - `--no-now` 安装并设置自启动，但不立即加载
  - `-h, --help` 显示帮助
- 安装流程：

  - 安装到 `/lib/modules/$(uname -r)/extra/crystal.ko`并运行`depmod`
  - 写入 `/etc/modules-load.d/crystal.conf` 配置开机自动加载
  - 立即加载（除非指定 `--no-now`）：`modprobe crystal` 或回退 `insmod`
  - 验证：`lsmod | grep crystal` 或 `dmesg | tail`
- 示例：

  - `./ins.sh`（安装并加载当前内核）
  - `./ins.sh --no-now`（仅安装并设置自启动，不立即加载）

## 运行演示

- 加载模块：`sudo insmod crystal.ko`（或执行 `./ins.sh` 完成安装与加载）
- 隐藏一个进程：`echo "add <PID>" | sudo tee /proc/crystal`
- 在 `ps`/`ls /proc` 的输出中，该 PID 对应的目录会被过滤（基于 `getdents64` 钩子）。
- 查看当前隐藏列表：`cat /proc/crystal`
- 卸载模块：`sudo rmmod crystal`

## 配置项

- 在 `rootkit.h` 中调整：
  - `MAGIC_WORD`：文件名包含该字符串时隐藏（默认 `"crystal"`）
  - `USER_HIDE` / `GROUP_HIDE`：当目录项属主/组匹配时隐藏

## 常见问题

- 未导出符号 `__get_task_ioprio`

  - 某些内核不导出该符号，已在 `get_current_process()` 中将 `ioprio` 固定为 `0`，避免 `modpost` 阶段的未定义符号错误。
  - 若仍报错，请 `make clean && make` 并确认正在使用最新产物。
- `/proc/crystal` 不存在或空

  - 确认模块已加载：`lsmod | grep crystal`
  - 查看日志：`dmesg | tail`（应有 `caraxes: /proc/crystal ready (read/write)`）
  - 添加隐藏后用 `cat /proc/crystal` 验证；`list` 命令输出到日志而非终端。
- Secure Boot 签名

  - 未签名模块可能无法加载。请为 `crystal.ko` 签名或暂时关闭 Secure Boot。

## Credits

- **sw1tchbl4d3/generic-linux-rootkit**：https://codeberg.org/sw1tchbl4d3/generic-linux-rootkit
- **Diamorphine**：目录项移除逻辑参考 https://github.com/m0nad/Diamorphine
- **ftrace_helper.h**：https://github.com/ilammy/ftrace-hook（调整为库以供复用）
- https://xcellerator.github.io/posts/linux_rootkits_01/ 提供了大量参考知识
