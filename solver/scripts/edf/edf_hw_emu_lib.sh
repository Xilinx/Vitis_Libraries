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
# Shared helpers for EDF (Embedded Device Flow) hw_emu.
#
# Platform differences arrive as EDF_* variables, exported by utils.mk. No board
# name is hardcoded below.
#
#   YOCTO_ARTIFACTS       root of the Yocto EDF artifacts tree            (required)
#   EDF_TOOLCHAIN_DIR     toolchain directory under YOCTO_ARTIFACTS       (required)
#   EDF_TOOLCHAIN_ENV     environment-setup script name inside the toolchain
#   EDF_QEMU_PREBUILT     QEMU prebuilt directory under YOCTO_ARTIFACTS   (hw_emu)
#   EDF_BOOT_BIN          EDF boot image basename
#   EDF_WIC_SUFFIX        rootfs image suffix: wic.qemu-sd | wic.ufs
#   EDF_FLASH_BIN         flash image basename: qemu-qspi.bin | qemu-ospi.bin
#   EDF_BOOT_MEDIA        where the boot image is installed: sd | ospi
#   EDF_WIC_SECTOR_SIZE   sector size for wic cp, empty means tool default
#   EDF_AIE_SIM_DIR       AIE sim device directory, empty disables the fixup
#   EDF_AIE_SIM_JSON      AIE sim device json, empty disables the fixup
#   EDF_SEGMENTED_XSA     on when v++ packages the PL alone, with no boot binary
#
set -euo pipefail

log_info() { echo "INFO: $*"; }
log_warn() { echo "WARNING: $*" >&2; }
log_error() { echo "ERROR: $*" >&2; }

EDF_TOOLCHAIN_ENV="${EDF_TOOLCHAIN_ENV:-environment-setup-cortexa72-cortexa53-amd-linux}"
EDF_WIC_SUFFIX="${EDF_WIC_SUFFIX:-wic.qemu-sd}"
EDF_BOOT_MEDIA="${EDF_BOOT_MEDIA:-sd}"

require_env() {
  local name
  for name in "$@"; do
    [ -n "${!name:-}" ] || { log_error "${name} is not set"; exit 1; }
  done
}

# v++ may emit a generic device json where a specific one is needed
fix_aie_sim_config() {
  local package_dir="${1:?package_dir}"
  local cfg="${package_dir}/cfg/aie.sim.config.txt"
  [ -n "${EDF_AIE_SIM_JSON:-}" ] || return 0
  [ -n "${EDF_AIE_SIM_DIR:-}" ] || return 0
  [ -f "${cfg}" ] || cfg="${package_dir}/sd_card/cfg/aie.sim.config.txt"
  [ -f "${cfg}" ] || return 0
  if awk -v want="${EDF_AIE_SIM_JSON}" \
      '/^JSON_DEVICE=/{if (index($0, want)) found=1} END{exit found ? 0 : 1}' "${cfg}"; then
    return 0
  fi
  {
    echo "DIRECTORY=${EDF_AIE_SIM_DIR}"
    echo "JSON_DEVICE=${EDF_AIE_SIM_JSON}"
    awk '!/^DIRECTORY=/ && !/^JSON_DEVICE=/' "${cfg}"
  } > "${cfg}.new"
  mv -f "${cfg}.new" "${cfg}"
  log_info "updated ${cfg} for ${EDF_AIE_SIM_JSON}"
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

# prefer the name from the platform table, fall back to a glob
resolve_edf_boot_name() {
  local dir="$1" preferred="${2:-${EDF_BOOT_BIN:-}}" name
  if [ -n "${preferred}" ] && [ -e "${dir}/${preferred}" ]; then
    echo "${preferred}"
    return 0
  fi
  name="$(pick_first_glob "${dir}" "BOOT-*-multidomain.bin")" && { echo "${name}"; return 0; }
  name="$(pick_first_glob "${dir}" "BOOT-*.bin")" && { echo "${name}"; return 0; }
  return 1
}

resolve_wic_image() {
  local dir="$1" name
  name="$(pick_first_glob "${dir}" "*.rootfs.${EDF_WIC_SUFFIX}")" \
    || { log_error "no *.rootfs.${EDF_WIC_SUFFIX} under ${dir}"; return 1; }
  echo "${name}"
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
  require_env YOCTO_ARTIFACTS EDF_TOOLCHAIN_DIR
  local setup="${YOCTO_ARTIFACTS}/${EDF_TOOLCHAIN_DIR}/sdk/${EDF_TOOLCHAIN_ENV}"
  [ -f "${setup}" ] || { log_error "EDF toolchain setup script not found: ${setup}"; exit 1; }
  unset LD_LIBRARY_PATH
  # shellcheck source=/dev/null
  source "${setup}"
  log_info "sourced EDF Yocto toolchain: ${EDF_TOOLCHAIN_DIR}"
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

is_segmented_xsa() {
  [ "${EDF_SEGMENTED_XSA:-}" = "on" ]
}

wic_cp_one() {
  local src="$1" dest="$2"
  local args=()
  [ -e "${src}" ] || { log_warn "skip missing: ${src}"; return 0; }
  [ -n "${EDF_WIC_SECTOR_SIZE:-}" ] && args+=("--sector-size=${EDF_WIC_SECTOR_SIZE}")
  wic cp "${args[@]}" "${src}" "${dest}"
  log_info "wic copied $(basename "${src}")"
}

# ospi: dd into the flash image, sd: wic cp into partition 1 of the WIC
install_boot_image() {
  local dir="$1" boot_src="$2"
  local boot_name dest wic

  boot_name="$(resolve_edf_boot_name "${dir}")" \
    || { log_warn "no EDF boot bin in ${dir}, skip boot install"; return 0; }

  dest="$(readlink -f "${dir}/${boot_name}")"
  rm -f "${dest}"
  cp -f "${boot_src}" "${dest}"

  case "${EDF_BOOT_MEDIA}" in
    ospi)
      require_env EDF_FLASH_BIN
      dd if="${dest}" of="${dir}/${EDF_FLASH_BIN}" conv=notrunc status=none
      log_info "boot: ${boot_src} -> ${dest}; refreshed ${EDF_FLASH_BIN}"
      ;;
    sd)
      wic="$(resolve_wic_image "${dir}")" || return 1
      wic_cp_one "${dest}" "${dir}/${wic}:1/boot.bin"
      log_info "boot: ${boot_src} -> ${dest}; installed into ${wic}:1/boot.bin"
      ;;
    *)
      log_error "unknown EDF_BOOT_MEDIA: ${EDF_BOOT_MEDIA}"
      return 1
      ;;
  esac
}

ensure_vpp_boot_bin_in_sd_card() {
  local pkg="$1" sd="$2" src
  if is_segmented_xsa && ! find_vpp_boot_bin "${sd}" >/dev/null && ! find_vpp_boot_bin "${pkg}" >/dev/null; then
    log_info "segmented XSA: no v++ boot binary to stage, keeping the EDF one"
    return 0
  fi
  [ -f "${sd}/BOOT.bin" ] && return 0
  src="$(find_vpp_boot_bin "${sd}")" \
    || src="$(find_vpp_boot_bin "${pkg}")" \
    || { log_error "v++ BOOT.bin not found under ${sd} or ${pkg}"; exit 1; }
  cp -af "${src}" "${sd}/BOOT.bin"
  log_info "copied ${src} -> ${sd}/BOOT.bin"
}

# linux_edf: the v++ BOOT.bin replaces the EDF one, then goes to the boot media.
install_vpp_boot() {
  local sd="$1" vpp_boot
  if is_segmented_xsa && ! find_vpp_boot_bin "${sd}" >/dev/null; then
    log_info "segmented XSA: keeping the EDF boot image, the PL arrives as a pdi/dtbo overlay"
    return 0
  fi
  vpp_boot="$(find_vpp_boot_bin "${sd}")" || {
    log_error "linux_edf: v++ BOOT.bin not found under ${sd} or _x/package"
    exit 1
  }
  install_boot_image "${sd}" "${vpp_boot}"
  rm -f "${vpp_boot}"
}

install_baremetal_pdi_boot() {
  local package_dir="$1" qemu_combined="$2" pdi_basename="$3" src
  if ! src="$(find_package_pdi "${package_dir}" "${pdi_basename}")"; then
    src="$(find_package_pdi "${package_dir}/sd_card" "${pdi_basename}")" \
      || { log_info "no package PDI under ${package_dir}, skip baremetal boot install"; return 0; }
  fi
  install_boot_image "${qemu_combined}" "${src}"
}
