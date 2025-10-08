#!/usr/bin/env bash
set -euo pipefail

# 仅安装指定版本的内核开发包与编译依赖，并确保构建链接可用
# 发行版支持：Ubuntu/Debian、RHEL/CentOS/Rocky/Alma、Fedora

KVER=""

usage() {
  cat <<EOF
用法: $0 [选项]

选项:
  -v, --version <KVER>    指定目标内核版本（默认: uname -r）
  -h, --help              显示帮助

示例:
  $0 --version 5.14.0-570.35.1.el9_6.x86_64
  $0                  # 使用当前内核版本
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -v|--version)
      KVER="${2:-}"; shift 2;;
    -h|--help)
      usage; exit 0;;
    *) echo "未知参数: $1" >&2; usage; exit 1;;
  esac
done

if [[ -z "${KVER}" ]]; then
  KVER="$(uname -r)"
fi

SUDO=""
if [[ "${EUID}" -ne 0 ]]; then
  SUDO="sudo"
fi

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
      # kernel-headers 可能为无版本名，尝试版本化安装失败后回退
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

main() {
  install_headers_and_deps
  ensure_build_link
  echo "完成: 已为 ${KVER} 安装内核开发包与依赖（如适用已修复构建链接）。"
  echo "下一步: 可在该主机上运行 ./build.sh --version ${KVER} 进行编译。"
}

main "$@"