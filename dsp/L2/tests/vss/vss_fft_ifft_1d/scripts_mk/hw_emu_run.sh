# Copyright (C) 2026, Advanced Micro Devices, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#!/bin/bash
# Embedded hw_emu launcher:
#   linux_edf   Linux + XRT on WIC: login, mount /dev/sda2, run embedded script
#   baremetal_ps gmio-style: PS ELF in package, launch + optional -aie-sim-options
#   legacy      pre-EDF embedded: launch_hw_emu.sh -run-app only
#
# Usage:
#   hw_emu_run.sh linux_edf <package_dir> <run_script> [qemu_conf]
#   hw_emu_run.sh baremetal_ps <package_dir> [qemu_conf] [aie_options_file]
#   hw_emu_run.sh legacy <package_dir> <run_script>
#
set -euo pipefail

FLOW="${1:?flow}"
PKG="${2:?package_dir}"
shift 2

resolve_launch_dir() {
  local pkg="$1"
  for candidate in "${pkg}/sd_card" "${pkg}"; do
    [ -x "${candidate}/launch_hw_emu.sh" ] && { echo "${candidate}"; return 0; }
  done
  echo "ERROR: launch_hw_emu.sh not found under ${pkg}" >&2
  return 1
}

resolve_qemu_conf() {
  local launch_dir="$1" conf="${2:-}"
  if [ -n "${conf}" ] && [ -f "${conf}" ]; then
    echo "${conf}"
    return 0
  fi
  if [ -f "${launch_dir}/combined.qemuboot.conf" ]; then
    echo "${launch_dir}/combined.qemuboot.conf"
    return 0
  fi
  echo "ERROR: combined.qemuboot.conf not found for ${launch_dir}" >&2
  return 1
}

case "${FLOW}" in
  linux_edf)
    RUN_SCRIPT="${1:?run_script}"
    QEMU_CONF="${2:-}"
    APP="$(basename "${RUN_SCRIPT}")"
    LAUNCH_DIR="$(resolve_launch_dir "${PKG}")"
    CONF="$(resolve_qemu_conf "${LAUNCH_DIR}" "${QEMU_CONF}")"
    cd "${LAUNCH_DIR}"
    exec ./launch_hw_emu.sh -qemu-config "${CONF}" -verbose \
      -login "amd-edf" -password "amd-edf" \
      -run-app "mount /dev/sda2 /media && cd /media && ./${APP}"
    ;;
  baremetal_ps|baremetal)
    QEMU_CONF="${1:-}"
    AIE_OPTIONS="${2:-}"
    LAUNCH_DIR="$(resolve_launch_dir "${PKG}")"
    CONF="$(resolve_qemu_conf "${LAUNCH_DIR}" "${QEMU_CONF}")"
    cd "${LAUNCH_DIR}"
    extra=()
    [ -n "${AIE_OPTIONS}" ] && [ -f "${AIE_OPTIONS}" ] && extra=(-aie-sim-options "${AIE_OPTIONS}")
    ENABLE_AIE_DBG_TRACE=true exec ./launch_hw_emu.sh -qemu-config "${CONF}" "${extra[@]}"
    ;;
  legacy)
    RUN_SCRIPT="${1:?run_script}"
    APP="$(basename "${RUN_SCRIPT}")"
    cd "${PKG}"
    exec ./launch_hw_emu.sh -no-reboot -run-app "${APP}"
    ;;
  *)
    echo "ERROR: unknown hw_emu flow: ${FLOW}" >&2
    exit 1
    ;;
esac
