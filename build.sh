#!/usr/bin/env bash
set -euo pipefail

# 模块目录（脚本所在目录，应与源码同级）
MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

KVER=""
CLEAN_FIRST=1
FORCE_MENU=0

SUDO=""
if [[ "${EUID}" -ne 0 ]]; then
  SUDO="sudo"
fi

usage() {
  cat <<EOF
用法: $0 [选项]

选项:
  -v, --version <KVER>    指定目标内核版本（如 5.14.0-570.35.1.el9_6.x86_64 或 6.8.0-52-generic）
  -m, --menu              打开菜单（选项：当前主机内核 / 手动输入 / 退出）
      --no-clean          不执行 clean，直接编译
  -h, --help              显示帮助

说明:
  - 若未指定版本，默认使用当前主机内核版本（uname -r），也可通过菜单选择。
  - 如果缺少 /lib/modules/<KVER>/build，脚本将自动安装匹配版本的内核开发包与依赖，并修复构建链接。
  - 支持 Ubuntu/Debian 与 RHEL/CentOS/Rocky/Alma/Fedora 的依赖安装与构建链接修复。
  - 编译成功后会将 crystal.ko 复制到项目目录 output/<KVER>/。
EOF
}

has_cmd() { command -v "$1" >/dev/null 2>&1; }

get_id() {
  if [[ -f /etc/os-release ]]; then
    . /etc/os-release
    echo "${ID:-}"
  else
    echo ""
  fi
}

install_headers_and_deps() {
  local id; id="$(get_id)"
  echo "检测到系统: ${id:-unknown}，目标内核: ${KVER}"
  case "$id" in
    ubuntu|debian)
      ${SUDO} apt-get update -y
      ${SUDO} apt-get install -y linux-headers-"${KVER}" build-essential || {
        echo "提示: 若找不到 linux-headers-${KVER}，请更新系统或安装相近版本的 headers 并切换到该内核。" >&2
        exit 2
      }
      ;;
    rhel|centos|rocky|almalinux)
      if has_cmd dnf; then PM=dnf; else PM=yum; fi
      ${SUDO} ${PM} install -y kernel-devel-"${KVER}" gcc make elfutils-libelf-devel || {
        echo "ERROR: 未找到匹配的 kernel-devel (${KVER})。请确保仓库启用并存在该版本。" >&2
        exit 2
      }
      if ! ${SUDO} ${PM} install -y kernel-headers-"${KVER}"; then
        ${SUDO} ${PM} install -y kernel-headers || true
      fi
      ;;
    fedora)
      ${SUDO} dnf install -y kernel-devel-"${KVER}" kernel-headers-"${KVER}" gcc make elfutils-libelf-devel || {
        echo "ERROR: 未找到匹配的 kernel-devel/kernel-headers (${KVER})。" >&2
        exit 2
      }
      ;;
    *)
      echo "ERROR: 未识别的发行版（/etc/os-release 缺失或不支持）。" >&2
      exit 1
      ;;
  esac
}

ensure_build_link() {
  local modules_dir="/lib/modules/${KVER}"
  local build_link="${modules_dir}/build"
  local src_kernel="/usr/src/kernels/${KVER}"
  local src_headers="/usr/src/linux-headers-${KVER}"
  local src=""

  # 选择可用的源码目录（RHEL/Fedora优先 /usr/src/kernels，Ubuntu/Debian 回退到 linux-headers）
  if [[ -d "${src_kernel}" ]]; then
    src="${src_kernel}"
  elif [[ -d "${src_headers}" ]]; then
    src="${src_headers}"
  fi

  # 确保 /lib/modules/<KVER> 目录存在
  if [[ ! -d "${modules_dir}" ]]; then
    echo "创建内核模块目录: ${modules_dir}"
    ${SUDO} mkdir -p "${modules_dir}"
  fi

  # 创建或修复 build 链接
  if [[ -L "${build_link}" && ! -e "${build_link}" ]]; then
    echo "检测到损坏的构建链接，移除后重建: ${build_link}"
    ${SUDO} rm -f "${build_link}"
  fi

  if [[ ! -e "${build_link}" ]]; then
    if [[ -n "${src}" ]]; then
      echo "修复构建链接: ${build_link} -> ${src}"
      ${SUDO} ln -sfn "${src}" "${build_link}"
    else
      echo "WARNING: 未找到可用的源码目录。RHEL/Fedora 请确认已安装 kernel-devel-${KVER}；Ubuntu/Debian 请确认已安装 linux-headers-${KVER}。" >&2
    fi
  fi
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

# 菜单选择函数（默认当前主机内核；提供手动输入与退出选项）
choose_version_menu() {
  local host_ver
  host_ver="$(uname -r)"
  echo "请选择要编译的内核版本："
  echo "  1) 当前主机内核 (${host_ver})"
  echo "  2) 手动输入版本"
  echo "  3) 退出"
  while true; do
    read -r -p "输入选项编号 [1-3] (默认 1): " sel
    if [[ -z "$sel" ]]; then sel=1; fi
    case "$sel" in
      1)
        KVER="$host_ver"; break ;;
      2)
        read -r -p "请输入完整版本字符串（例如 5.14.0-570.35.1.el9_6.x86_64 或 6.8.0-79-generic）: " manual
        if [[ -n "$manual" ]]; then KVER="$manual"; break; else echo "输入为空，请重试。"; fi ;;
      3)
        echo "已退出。"; exit 0 ;;
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

# 若缺少构建目录，自动安装依赖并修复链接
KDIR="/lib/modules/${KVER}/build"
if [[ ! -d "$KDIR" ]]; then
  echo "未找到内核头文件目录: $KDIR，尝试安装依赖并修复链接..."
  install_headers_and_deps
  ensure_build_link
fi

# 重新检查构建目录
if [[ ! -d "$KDIR" ]]; then
  echo "ERROR: 仍未找到内核头文件目录: $KDIR" >&2
  echo "建议:" >&2
  echo " - 确认仓库中存在该版本的 kernel-devel/linux-headers。" >&2
  echo " - RHEL/Fedora 检查 /usr/src/kernels/${KVER} 是否存在；Ubuntu/Debian 检查 /usr/src/linux-headers-${KVER}。" >&2
  echo " - 如无法安装匹配版本，可切换到提供该版本的主机再编译。" >&2
  exit 2
fi

# 简单校验：确保源码目录存在 Makefile
if [[ ! -f "$MODULE_DIR/Makefile" ]]; then
  echo "ERROR: 未在 $MODULE_DIR 找到 Makefile。请确保脚本与源码同目录。" >&2
  exit 3
fi

echo "开始为内核版本: ${KVER} 在主机上编译 crystal 模块"
echo "使用头文件目录: $KDIR"

if [[ "$CLEAN_FIRST" -eq 1 ]]; then
  make -C "$KDIR" M="$MODULE_DIR" clean
fi

make -C "$KDIR" M="$MODULE_DIR" modules

if [[ -f "$MODULE_DIR/crystal.ko" ]]; then
  echo "完成: 已为 ${KVER} 编译模块 -> $MODULE_DIR/crystal.ko"
  # 复制到项目当前目录的对应内核目录
  local_out_dir="${MODULE_DIR}/output/${KVER}"
  echo "复制模块到项目目录: ${local_out_dir}/crystal.ko"
  mkdir -p "${local_out_dir}"
  cp -f "$MODULE_DIR/crystal.ko" "${local_out_dir}/crystal.ko"
else
  echo "警告: 编译结束但未找到 crystal.ko，请检查输出与错误信息。" >&2
fi