#!/usr/bin/env bash
set -euo pipefail

# 模块目录（脚本所在目录，应与源码同级）
MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

KVER=""
CLEAN_FIRST=1
FORCE_MENU=0

usage() {
  cat <<EOF
用法: $0 [选项]

选项:
  -v, --version <KVER>    指定目标内核版本（如 6.8.0-52-generic）
      --generic-6.8       快速选择 6.8.0-52-generic
      --generic-6.8-79    快速选择 6.8.0-79-generic
  -m, --menu              进入交互菜单选择版本
      --no-clean          不执行 clean，直接编译
  -h, --help              显示帮助

说明:
  - 在当前 Ubuntu 主机上编译，不使用 Docker。
  - 脚本与源码同目录，自动使用该目录作为模块路径。
  - 若未指定版本，默认使用当前主机内核版本（uname -r）。
  - 也可通过菜单选择：当前主机内核、6.8.0-52-generic、6.8.0-79-generic或手动输入。
  - 需要存在 /lib/modules/<KVER>/build，否则无法编译，请在主机安装匹配的 headers。
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -v|--version)
      KVER="${2:-}"
      if [[ -z "${KVER}" ]]; then
        echo "ERROR: 未提供版本号给 --version" >&2
        exit 1
      fi
      shift 2
      ;;
    --generic-6.8)
      KVER="6.8.0-52-generic"
      shift
      ;;
    --generic-6.8-79)
      KVER="6.8.0-79-generic"
      shift
      ;;
    -m|--menu)
      FORCE_MENU=1
      shift
      ;;
    --no-clean)
      CLEAN_FIRST=0
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "未知参数: $1" >&2
      usage
      exit 1
      ;;
  esac
done

# 菜单选择函数
choose_version_menu() {
  local host_ver
  host_ver="$(uname -r)"
  echo "请选择要编译的内核版本："
  echo "  1) 当前主机内核 (${host_ver})"
  echo "  2) 6.8.0-52-generic"
  echo "  3) 6.8.0-79-generic"
  echo "  4) 手动输入版本"
  while true; do
    read -r -p "输入选项编号 [1-4]: " sel
    case "$sel" in
      1)
        KVER="$host_ver"; break ;;
      2)
        KVER="6.8.0-52-generic"; break ;;
      3)
        KVER="6.8.0-79-generic"; break ;;
      4)
        read -r -p "请输入完整版本字符串（例如 6.8.0-79-generic）: " manual
        if [[ -n "$manual" ]]; then KVER="$manual"; break; fi ;;
      *) echo "无效选择，请重试。" ;;
    esac
  done
}

# 若未指定版本或强制菜单，则进入菜单；否则默认使用当前主机内核版本
if [[ -z "${KVER}" || "${FORCE_MENU}" -eq 1 ]]; then
  choose_version_menu
fi
if [[ -z "${KVER}" ]]; then
  KVER="$(uname -r)"
fi

# 头文件目录
KDIR="/lib/modules/${KVER}/build"
if [[ ! -d "$KDIR" ]]; then
  echo "ERROR: 未找到内核头文件目录: $KDIR" >&2
  echo "提示:" >&2
  echo " - 请安装匹配的头文件: sudo apt-get update && sudo apt-get install linux-headers-${KVER}" >&2
  echo " - 或切换到拥有 /lib/modules/${KVER}/build 的机器上运行。" >&2
  exit 2
fi

# 简单校验：确保源码目录存在 Makefile
if [[ ! -f "$MODULE_DIR/Makefile" ]]; then
  echo "ERROR: 未在 $MODULE_DIR 找到 Makefile。请确保脚本与源码同目录。" >&2
  exit 3
fi

echo "开始为内核版本: ${KVER} 在主机上编译 caraxes 模块"
echo "使用头文件目录: $KDIR"

if [[ "$CLEAN_FIRST" -eq 1 ]]; then
  make -C "$KDIR" M="$MODULE_DIR" clean
fi

make -C "$KDIR" M="$MODULE_DIR" modules

if [[ -f "$MODULE_DIR/caraxes.ko" ]]; then
  echo "完成: 已为 ${KVER} 编译模块 -> $MODULE_DIR/caraxes.ko"
else
  echo "警告: 编译结束但未找到 caraxes.ko，请检查输出与错误信息。" >&2
fi