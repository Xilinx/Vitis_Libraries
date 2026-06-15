#!/bin/bash
# Release NFS locks on package.hw_emu/sim before v++ repackage or hw_emu rerun.
#
# Usage:
#   edf_pre_package_cleanup.sh <package_dir>           # kill + remove sim/
#   edf_pre_package_cleanup.sh <package_dir> kill-only # kill only (before launch)
#
set -euo pipefail

log_info() { echo "INFO: $*"; }
log_warn() { echo "WARNING: $*" >&2; }
log_error() { echo "ERROR: $*" >&2; }

PACKAGE_DIR="${1:?package_dir}"
MODE="${2:-full}"
PKG="$(readlink -f "${PACKAGE_DIR}" 2>/dev/null || echo "${PACKAGE_DIR}")"

_hw_emu_proc_match() {
  local cmdline="$1" pkg="$2" cwd="${3:-}"
  case "${cmdline}" in
    *launch_hw_emu*|*runqemu*|*qemu-system*|*xsim*|*xelab*|*xsc*)
      [[ "${cmdline}" == *"${pkg}"* || "${cwd}" == "${pkg}"* ]]
      ;;
    *) return 1 ;;
  esac
}

kill_stale_hw_emu_procs() {
  local pkg="$1" pid cmdline cwd sig
  for sig in TERM KILL; do
    for pid in $(pgrep -u "$(id -u)" 2>/dev/null || true); do
      [ -d "/proc/${pid}" ] || continue
      [ "${pid}" -eq "$$" ] && continue
      cmdline="$(tr '\0' ' ' < "/proc/${pid}/cmdline" 2>/dev/null || true)"
      cwd="$(readlink -f "/proc/${pid}/cwd" 2>/dev/null || true)"
      _hw_emu_proc_match "${cmdline}" "${pkg}" "${cwd}" || continue
      kill "-${sig}" "${pid}" 2>/dev/null || true
    done
    sleep "$([ "${sig}" = TERM ] && echo 2 || echo 1)"
  done
}

remove_sim_dir() {
  local sim="$1" attempt stale
  [ -d "${sim}" ] || return 0

  if command -v fuser >/dev/null 2>&1; then
    fuser -k -TERM "${sim}" 2>/dev/null || true
    sleep 2
    fuser -k -KILL "${sim}" 2>/dev/null || true
    sleep 1
  fi

  for attempt in 1 2 3 4 5 6; do
    rm -rf "${sim}" 2>/dev/null && { log_info "removed ${sim}"; return 0; }
    find "${sim}" -name '.nfs*' -type f -delete 2>/dev/null || true
    sleep 2
  done

  stale="${sim}.stale.$$"
  if mv "${sim}" "${stale}" 2>/dev/null; then
    log_warn "moved busy ${sim} -> ${stale}"
    return 0
  fi

  log_error "cannot clear ${sim} (device busy / NFS .nfs files)"
  return 1
}

kill_stale_hw_emu_procs "${PKG}"

if [ "${MODE}" = "kill-only" ]; then
  log_info "hw_emu process cleanup done for ${PKG}"
  exit 0
fi

remove_sim_dir "${PKG}/sim"
remove_sim_dir "${PKG}/sd_card/sim"

log_info "pre-package cleanup done for ${PKG}"
