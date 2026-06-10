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
# Shared helpers for vek385 EDF hw_emu (QEMU basecamp, WIC deploy, boot/OSPI).
set -euo pipefail

log_info() { echo "INFO: $*"; }
log_warn() { echo "WARNING: $*" >&2; }
log_error() { echo "ERROR: $*" >&2; }

EDF_BOOT_DEFAULT="${EDF_BOOT_DEFAULT:-BOOT-versal-2ve-2vm-vek385-revb-multidomain.bin}"
EDF_OSPI_DEFAULT="${EDF_OSPI_DEFAULT:-qemu-ospi.bin}"
# v++ package may emit legacy 4x4.json; AIE2PS vek385 hw_emu needs XC2VE3858.json.
fix_aie_sim_config() {
  local package_dir="${1:?package_dir}"
  local cfg="${package_dir}/cfg/aie.sim.config.txt"
  [ -f "${cfg}" ] || cfg="${package_dir}/sd_card/cfg/aie.sim.config.txt"
  [ -f "${cfg}" ] || return 0
  if ! awk '/^JSON_DEVICE=/{if ($0 ~ /4x4\.json/) found=1} END{exit found ? 0 : 1}' "${cfg}"; then
    return 0
  fi
  {
    echo "DIRECTORY=data/aie2ps/devices"
    echo "JSON_DEVICE=XC2VE3858.json"
    awk '!/^DIRECTORY=/ && !/^JSON_DEVICE=/' "${cfg}"
  } > "${cfg}.new"
  mv -f "${cfg}.new" "${cfg}"
  log_info "updated ${cfg} for AIE2PS (XC2VE3858.json)"
}

pick_first_in_dir() {
  local dir="$1" name
  shift
  for name in "$@"; do
    if [ -e "${dir}/${name}" ]; then
      echo "${name}"
      return 0
    fi
  done
  return 1
}

pick_first_glob() {
  local dir="$1" pattern="$2" path
  shopt -s nullglob
  for path in "${dir}"/${pattern}; do
    [ -e "${path}" ] || continue
    basename "${path}"
    shopt -u nullglob
    return 0
  done
  shopt -u nullglob
  return 1
}

resolve_edf_boot_name() {
  local dir="$1" preferred="${2:-${EDF_BOOT_DEFAULT}}" name
  [ -e "${dir}/${preferred}" ] && { echo "${preferred}"; return 0; }
  if name="$(pick_first_in_dir "${dir}" \
      "BOOT-versal-2ve-2vm-vek385-revb-multidomain.bin" \
      "BOOT-versal-2ve-2vm-vek385-multidomain.bin")"; then
    echo "${name}"
    return 0
  fi
  name="$(pick_first_glob "${dir}" "BOOT-versal-2ve-2vm-vek385*-multidomain*.bin")" \
    && { echo "${name}"; return 0; }
  return 1
}

find_package_pdi() {
  local pkg="$1" hint="${2:-}" f
  if [ -n "${hint}" ] && [ -f "${pkg}/${hint}" ]; then
    echo "${pkg}/${hint}"
    return 0
  fi
  if [ -n "${hint}" ] && [[ "${hint}" != */* ]] && [ -f "${pkg}/${hint}.pdi" ]; then
    echo "${pkg}/${hint}.pdi"
    return 0
  fi
  for f in "${pkg}/a.pdi" "${pkg}/kernel.pdi" "${pkg}/BOOT.bin"; do
    [ -f "${f}" ] && { echo "${f}"; return 0; }
  done
  shopt -s nullglob
  for f in "${pkg}"/*.pdi; do
    [[ "${f}" == *presynth* ]] && continue
    echo "${f}"
    shopt -u nullglob
    return 0
  done
  shopt -u nullglob
  return 1
}

find_vpp_boot_bin() {
  local sd="$1" f
  for f in "${sd}/BOOT.bin" \
           "${sd}/../_x/package/BOOT.bin" \
           "${sd}/../../_x/package/BOOT.bin"; do
    [ -f "${f}" ] && [ ! -L "${f}" ] && { echo "${f}"; return 0; }
  done
  [ -f "${sd}/boot.bin" ] && [ ! -L "${sd}/boot.bin" ] && { echo "${sd}/boot.bin"; return 0; }
  return 1
}

source_edf_yocto_sdk() {
  [ -n "${YOCTO_ARTIFACTS:-}" ] || { log_error "YOCTO_ARTIFACTS is not set"; exit 1; }
  local setup="${YOCTO_ARTIFACTS}/amd-cortexa78-mali-common_meta-edf-app-sdk/sdk/environment-setup-cortexa72-cortexa53-amd-linux"
  [ -f "${setup}" ] || { log_error "EDF SDK setup script not found: ${setup}"; exit 1; }
  unset LD_LIBRARY_PATH
  # shellcheck source=/dev/null
  source "${setup}"
  log_info "sourced EDF Yocto SDK"
}

source_qemuboot_tool_env() {
  local env_setup
  if [ -n "${XILINX_VITIS:-}" ]; then
    for env_setup in \
      "${XILINX_VITIS}/data/emulation/qemu/comp/qemu_edf/environment-setup-x86_64-amdedfsdk-linux" \
      "${XILINX_VITIS}/data/emulation/qemu/comp/qemu/environment-setup-x86_64-petalinux-linux"; do
      if [ -f "${env_setup}" ]; then
        unset LD_LIBRARY_PATH
        # shellcheck source=/dev/null
        source "${env_setup}"
        log_info "sourced $(basename "${env_setup}") for qemuboot-tool"
        return 0
      fi
    done
  fi
  command -v qemuboot-tool >/dev/null 2>&1
}

is_linux_edf_sd_card() {
  [ -f "${1}/launch_hw_emu.sh" ]
}

install_boot_on_ospi() {
  local qemu_dir="$1" boot_src="$2"
  local edf_boot="${3:-${EDF_BOOT_DEFAULT}}"
  local ospi="${4:-${EDF_OSPI_DEFAULT}}"
  local boot_name dest

  boot_name="$(resolve_edf_boot_name "${qemu_dir}" "${edf_boot}")" \
    || { log_warn "no EDF boot bin in ${qemu_dir}, skip OSPI refresh"; return 0; }

  dest="$(readlink -f "${qemu_dir}/${boot_name}")"
  rm -f "${dest}"
  cp -f "${boot_src}" "${dest}"
  dd if="${dest}" of="${qemu_dir}/${ospi}" conv=notrunc status=none
  log_info "boot: ${boot_src} -> ${dest}; refreshed ${ospi}"
}

ensure_vpp_boot_bin_in_sd_card() {
  local pkg="$1" sd="$2" src
  [ -f "${sd}/BOOT.bin" ] && return 0
  src="$(find_vpp_boot_bin "${sd}")" \
    || src="$(find_vpp_boot_bin "${pkg}")" \
    || { log_error "v++ BOOT.bin not found under ${sd} or ${pkg}"; exit 1; }
  cp -af "${src}" "${sd}/BOOT.bin"
  log_info "copied ${src} -> ${sd}/BOOT.bin"
}

install_vpp_boot_on_ospi() {
  local sd="$1"
  local edf_boot="${2:-${EDF_BOOT_DEFAULT}}"
  local ospi="${3:-${EDF_OSPI_DEFAULT}}"
  local vpp_boot dest

  vpp_boot="$(find_vpp_boot_bin "${sd}")" || {
    log_error "linux_edf: v++ BOOT.bin not found under ${sd} or _x/package"
    exit 1
  }

  [ -e "${sd}/${edf_boot}" ] \
    || edf_boot="$(pick_first_in_dir "${sd}" \
         "BOOT-versal-2ve-2vm-vek385-revb-multidomain.bin" \
         "BOOT-versal-2ve-2vm-vek385-multidomain.bin")" \
    || edf_boot="$(pick_first_glob "${sd}" "BOOT-versal-2ve-2vm-vek385*-multidomain*.bin")" \
    || { log_error "linux_edf: EDF boot bin missing in ${sd}"; exit 1; }

  dest="$(readlink -f "${sd}/${edf_boot}")"
  mv -f "${vpp_boot}" "${dest}"
  dd if="${dest}" of="${sd}/${ospi}" conv=notrunc status=none
  log_info "linux_edf boot: moved ${vpp_boot} -> ${dest}; refreshed ${ospi}"
}

install_baremetal_pdi_boot() {
  local package_dir="$1" qemu_combined="$2" pdi_basename="$3"
  local edf_boot="${4:-${EDF_BOOT_DEFAULT}}"
  local ospi="${5:-${EDF_OSPI_DEFAULT}}"
  local src

  if ! src="$(find_package_pdi "${package_dir}" "${pdi_basename}")"; then
    src="$(find_package_pdi "${package_dir}/sd_card" "${pdi_basename}")" \
      || { log_info "no package PDI under ${package_dir}, skip baremetal boot install"; return 0; }
  fi
  install_boot_on_ospi "${qemu_combined}" "${src}" "${edf_boot}" "${ospi}"
}

# Route QEMU mon:stdio to the 4th UART (ttyAMA1) in combined.qemuboot.conf.
fixup_qemu_serial() {
  local qemu_combined="${1:?qemu_combined_dir}"
  local conf="${qemu_combined}/combined.qemuboot.conf"
  local wrong='-serial null -serial null -serial mon:stdio -serial null'
  local right='-serial null -serial null -serial null -serial mon:stdio'
  local conf_content tmp
  conf_content="$(<"${conf}")"
  tmp="$(mktemp "${TMPDIR:-/tmp}/edf-qemu-serial.XXXXXX")"
  printf '%s' "${conf_content//${wrong}/${right}}" > "${tmp}"
  mv -f "${tmp}" "${conf}"
  log_info "moved mon:stdio to 4th serial (ttyAMA1) in ${conf}"
}
