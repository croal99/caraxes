#!/usr/bin/env bash
set -euo pipefail

# Install crystal.ko for the current kernel, set autoload at boot,
# and optionally load immediately. No compilation is performed.

KVER=""
LOAD_NOW=1

usage() {
  cat <<EOF
Usage: $0 [options]

Options:
  --no-now                Do not load the module immediately after install
  -h, --help              Show this help

Examples:
  ./ins.sh                        # Install for current kernel, set autoload, and load now
  ./ins.sh --no-now               # Install and set autoload, but skip immediate load

Note: This script does NOT compile. Ensure crystal.ko exists in the current directory.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --no-now)
      LOAD_NOW=0; shift;;
    -h|--help)
      usage; exit 0;;
    *) echo "Unknown argument: $1" >&2; usage; exit 1;;
  esac
done

KVER="$(uname -r)"

SUDO=""
if [[ "${EUID}" -ne 0 ]]; then
  SUDO="sudo"
fi

has_cmd() { command -v "$1" >/dev/null 2>&1; }

ensure_crystal_ko() {
  if [[ ! -f "crystal.ko" ]]; then
    echo "ERROR: crystal.ko not found. This script does not compile; place the built module in the current directory." >&2
    exit 1
  fi
}

install_module() {
  local dest_dir="/lib/modules/${KVER}/extra"
  local dest_mod="${dest_dir}/crystal.ko"
  echo "Install module to ${dest_mod}"
  ${SUDO} mkdir -p "${dest_dir}"
  ${SUDO} install -m 644 "crystal.ko" "${dest_mod}"
  echo "Run depmod"
  ${SUDO} depmod -a "${KVER}"
}

setup_autoload() {
  echo "Configure boot-time autoload: /etc/modules-load.d/crystal.conf"
  echo crystal | ${SUDO} tee /etc/modules-load.d/crystal.conf >/dev/null
}

load_now() {
  if [[ "${LOAD_NOW}" -eq 1 ]]; then
    echo "Load module now"
    if has_cmd modprobe; then
      if ! ${SUDO} modprobe crystal; then
        echo "modprobe failed, trying insmod"
        if ! ${SUDO} insmod "/lib/modules/${KVER}/extra/crystal.ko"; then
          echo "ERROR: Load failed. If Secure Boot is enabled, sign the module or temporarily disable it." >&2
          exit 2
        fi
      fi
    else
      if ! ${SUDO} insmod "/lib/modules/${KVER}/extra/crystal.ko"; then
        echo "ERROR: Load failed. If Secure Boot is enabled, sign the module or temporarily disable it." >&2
        exit 2
      fi
    fi
    # Refresh systemd modules-load service if present
    if has_cmd systemctl; then
      ${SUDO} systemctl restart systemd-modules-load || true
    fi
    echo "Loaded. Verify with: lsmod | grep crystal or dmesg | tail"
  else
    echo "Skipped immediate load (--no-now)"
  fi
}

main() {
  ensure_crystal_ko
  install_module
  setup_autoload
  load_now
  echo "Done: crystal installed to /lib/modules/${KVER}/extra and set to autoload on boot."
}

main "$@"