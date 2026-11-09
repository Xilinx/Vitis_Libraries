#!/bin/bash
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
# Prepare the QEMU basecamp for EDF hw_emu (prebuilt copy, qemuboot merge, boot, serial).
#
# Usage: edf_hw_emu_prepare.sh <package_dir> <qemu_combined_dir> [pdi_basename]
#
# Platform parameters come from the EDF_* environment, see edf_hw_emu_lib.sh.
#
# Flow (by 3rd arg / directory layout):
#   pdi_basename set        -> baremetal_ps: merge prebuilt, install PDI as boot image
#   sd_card (launch_hw_emu) -> linux_edf: merge yocto WIC/QEMU only; boot install deferred
#                              to edf_wic_deploy, which runs after the app files are in
#   otherwise               -> legacy sd_card: merge prebuilt + qemuboot only
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=edf_hw_emu_lib.sh
source "${SCRIPT_DIR}/edf_hw_emu_lib.sh"

PACKAGE_DIR="${1:?package_dir}"
QEMU_COMBINED="${2:?qemu_combined_dir}"
PDI_BASENAME="${3:-}"

require_env YOCTO_ARTIFACTS EDF_QEMU_PREBUILT

PREBUILT_SRC="${YOCTO_ARTIFACTS}/${EDF_QEMU_PREBUILT}"

copy_qemu_prebuilt() {
  [ -d "${PREBUILT_SRC}" ] || { log_error "QEMU prebuilt not found: ${PREBUILT_SRC}"; exit 1; }
  mkdir -p "${QEMU_COMBINED}"
  cp -prf "${PREBUILT_SRC}/." "${QEMU_COMBINED}/"
  log_info "merged QEMU prebuilt from ${PREBUILT_SRC} into ${QEMU_COMBINED}"
}

# merge only the yocto WIC/QEMU assets, keep the v++ cosim sim/ and cfg/
merge_prebuilt_linux_edf() {
  local item base
  [ -d "${PREBUILT_SRC}" ] || { log_error "QEMU prebuilt not found: ${PREBUILT_SRC}"; exit 1; }
  mkdir -p "${QEMU_COMBINED}"

  # --sparse: the rootfs WIC is mostly holes, writing them out costs GBs per case
  if command -v rsync >/dev/null 2>&1; then
    rsync -a --sparse --ignore-existing --exclude 'sim/' --exclude 'cfg/' \
      "${PREBUILT_SRC}/" "${QEMU_COMBINED}/"
  else
    shopt -s dotglob nullglob
    for item in "${PREBUILT_SRC}"/*; do
      base="$(basename "${item}")"
      case "${base}" in sim|cfg) continue ;; esac
      [ -e "${QEMU_COMBINED}/${base}" ] && continue
      cp -a "${item}" "${QEMU_COMBINED}/"
    done
    shopt -u dotglob nullglob
  fi

  # rsync --ignore-existing keeps the v++ copies, so force-refresh the yocto assets.
  shopt -s nullglob
  local refresh=(
    "${PREBUILT_SRC}"/*.rootfs."${EDF_WIC_SUFFIX}"
    "${PREBUILT_SRC}"/combined.qemuboot.conf
    "${PREBUILT_SRC}"/BOOT-*.bin
    "${PREBUILT_SRC}"/BOOT-*.qemuboot.conf
    "${PREBUILT_SRC}"/edf-platform-disk-image-*.qemuboot.conf
  )
  shopt -u nullglob
  [ -n "${EDF_FLASH_BIN:-}" ] && refresh+=("${PREBUILT_SRC}/${EDF_FLASH_BIN}")
  for item in "${refresh[@]}"; do
    [ -f "${item}" ] || continue
    cp -af "${item}" "${QEMU_COMBINED}/$(basename "${item}")"
  done

  resolve_wic_image "${QEMU_COMBINED}" >/dev/null \
    || { log_error "no rootfs image after yocto prebuilt merge in ${QEMU_COMBINED}"; exit 1; }

  log_info "merged yocto WIC/QEMU prebuilt into ${QEMU_COMBINED} (sim/cfg and v++ BOOT/kernel preserved)"
}

ensure_qemuboot_conf() {
  local conf="${QEMU_COMBINED}/combined.qemuboot.conf"
  local boot_conf rootfs_conf

  if ! source_qemuboot_tool_env || ! command -v qemuboot-tool >/dev/null 2>&1; then
    [ -f "${conf}" ] || { log_error "missing ${conf} and qemuboot-tool unavailable"; exit 1; }
    log_warn "qemuboot-tool unavailable; using prebuilt ${conf}"
    return 0
  fi

  boot_conf="$(pick_first_in_dir "${QEMU_COMBINED}" "${EDF_BOOT_BIN%.bin}.qemuboot.conf")" \
    || boot_conf="$(pick_first_glob "${QEMU_COMBINED}" "BOOT-*-multidomain.qemuboot.conf")" \
    || { log_error "BOOT qemuboot.conf not found in ${QEMU_COMBINED}"; exit 1; }

  rootfs_conf="$(pick_first_glob "${QEMU_COMBINED}" "edf-platform-disk-image-*.rootfs.qemuboot.conf")" \
    || rootfs_conf="$(pick_first_glob "${QEMU_COMBINED}" "edf-platform-disk-image-*.rootfs*.qemuboot.conf")" \
    || { log_error "rootfs qemuboot.conf not found in ${QEMU_COMBINED}"; exit 1; }

  rm -f "${conf}"
  (
    cd "${QEMU_COMBINED}"
    qemuboot-tool \
      load "${boot_conf}" \
      remove image_link_name \
      remove image_name \
      merge "${rootfs_conf}" \
      remove staging_bindir_native \
      remove staging_dir_host \
      remove staging_dir_native \
      remove uninative_loader \
      > combined.qemuboot.conf
  )
  log_info "generated ${conf}"
}

fix_aie_sim_config "${PACKAGE_DIR}"

if is_linux_edf_sd_card "${QEMU_COMBINED}"; then
  merge_prebuilt_linux_edf
  ensure_vpp_boot_bin_in_sd_card "${PACKAGE_DIR}" "${QEMU_COMBINED}"
  log_info "linux_edf sd_card: yocto WIC/QEMU prebuilt merged; boot install deferred to edf_wic_deploy"
elif [ -n "${PDI_BASENAME}" ]; then
  copy_qemu_prebuilt
  install_baremetal_pdi_boot "${PACKAGE_DIR}" "${QEMU_COMBINED}" "${PDI_BASENAME}"
else
  copy_qemu_prebuilt
  ensure_qemuboot_conf
fi

log_info "edf_hw_emu_prepare completed"
