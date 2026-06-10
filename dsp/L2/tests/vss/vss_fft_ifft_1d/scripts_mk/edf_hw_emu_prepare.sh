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
# Prepare QEMU basecamp for AIE2PS hw_emu (prebuilt copy, qemuboot merge, OSPI, serial).
#
# Usage: edf_hw_emu_prepare.sh <package_dir> <qemu_combined_dir> \
#           [prebuilt_subdir] [edf_boot_bin] [qemu_ospi_bin] [pdi_basename]
#
# Flow (by 6th arg / path):
#   pdi_basename set        -> baremetal_ps: merge prebuilt, install PDI on OSPI
#   sd_card (launch_hw_emu) -> linux_edf: merge yocto WIC/QEMU only; BOOT on OSPI in wic deploy
#   otherwise               -> legacy sd_card: merge prebuilt + qemuboot only
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=edf_hw_emu_lib.sh
source "${SCRIPT_DIR}/edf_hw_emu_lib.sh"

PACKAGE_DIR="${1:?package_dir}"
QEMU_COMBINED="${2:?qemu_combined_dir}"
QEMU_PREBUILT="${3:-amd-cortexa78-mali-common_vek385revb_qemu_prebuilt}"
EDF_BOOT_BIN="${4:-${EDF_BOOT_DEFAULT}}"
QEMU_OSPI_BIN="${5:-${EDF_OSPI_DEFAULT}}"
PDI_BASENAME="${6:-}"

copy_qemu_prebuilt() {
  local src="${YOCTO_ARTIFACTS}/${QEMU_PREBUILT}"
  [ -d "${src}" ] || { log_error "QEMU prebuilt not found: ${src}"; exit 1; }
  mkdir -p "${QEMU_COMBINED}"
  cp -prf "${src}/." "${QEMU_COMBINED}/"
  log_info "merged QEMU prebuilt from ${src} into ${QEMU_COMBINED}"
}

# v++ sd_card has kernel/sim/cfg/BOOT.bin (sim is read-only on NFS Jenkins).
# Merge only yocto WIC/QEMU assets; never copy prebuilt sim/ or cfg/ over v++ cosim layout.
merge_prebuilt_linux_edf() {
  local src="${YOCTO_ARTIFACTS}/${QEMU_PREBUILT}"
  local item base
  [ -d "${src}" ] || { log_error "QEMU prebuilt not found: ${src}"; exit 1; }
  mkdir -p "${QEMU_COMBINED}"

  if command -v rsync >/dev/null 2>&1; then
    rsync -a --ignore-existing --exclude 'sim/' --exclude 'cfg/' "${src}/" "${QEMU_COMBINED}/"
  else
    shopt -s dotglob nullglob
    for item in "${src}"/*; do
      base="$(basename "${item}")"
      case "${base}" in sim|cfg) continue ;; esac
      [ -e "${QEMU_COMBINED}/${base}" ] && continue
      cp -a "${item}" "${QEMU_COMBINED}/"
    done
    shopt -u dotglob nullglob
  fi

  shopt -s nullglob
  for item in \
    "${src}"/*.rootfs.wic.ufs \
    "${src}"/combined.qemuboot.conf \
    "${src}"/qemu-ospi.bin \
    "${src}"/BOOT-versal-2ve-2vm-vek385*.bin \
    "${src}"/BOOT-versal-2ve-2vm-vek385*.qemuboot.conf \
    "${src}"/edf-platform-disk-image-*.qemuboot.conf; do
    [ -e "${item}" ] || continue
    cp -af "${item}" "${QEMU_COMBINED}/$(basename "${item}")"
  done
  shopt -u nullglob

  pick_first_glob "${QEMU_COMBINED}" "*.rootfs.wic.ufs" \
    || { log_error "no *.rootfs.wic.ufs after yocto prebuilt merge in ${QEMU_COMBINED}"; exit 1; }

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

  boot_conf="$(pick_first_in_dir "${QEMU_COMBINED}" \
    "BOOT-versal-2ve-2vm-vek385-revb-multidomain.qemuboot.conf" \
    "BOOT-versal-2ve-2vm-vek385-multidomain.qemuboot.conf")" \
    || pick_first_glob "${QEMU_COMBINED}" "BOOT-versal-2ve-2vm-vek385*-multidomain*.qemuboot.conf" \
    || { log_error "BOOT qemuboot.conf not found in ${QEMU_COMBINED}"; exit 1; }

  rootfs_conf="$(pick_first_in_dir "${QEMU_COMBINED}" \
    "edf-platform-disk-image-amd-cortexa78-mali-common.rootfs.qemuboot.conf")" \
    || pick_first_glob "${QEMU_COMBINED}" "edf-platform-disk-image-amd-cortexa78-mali-common.rootfs*.qemuboot.conf" \
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
  install_baremetal_pdi_boot "${PACKAGE_DIR}" "${QEMU_COMBINED}" "${PDI_BASENAME}" \
    "${EDF_BOOT_BIN}" "${QEMU_OSPI_BIN}"
else
  copy_qemu_prebuilt
  ensure_qemuboot_conf
fi

fixup_qemu_serial "${QEMU_COMBINED}"
log_info "edf_hw_emu_prepare completed"
