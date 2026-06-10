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
# Deploy application artifacts into WIC rootfs for Linux EDF hw_emu.
#
# Usage:
#   edf_wic_deploy.sh <xclbin> <host_exe> <dtbo> <pdi> <run_script> \
#       <qemu_combined_dir> <wic_partition> <data_dir> <emconfig> [data_files...]
#
# Environment: XCLBIN_LOOKUP_NAMES - extra xclbin alias basenames (no .xclbin)
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=edf_hw_emu_lib.sh
source "${SCRIPT_DIR}/edf_hw_emu_lib.sh"

XCLBIN="${1:?xclbin}"
HOST_EXE="${2:?host_exe}"
DTBO="${3:?dtbo}"
PDI="${4:?pdi}"
RUN_SCRIPT="${5:?run_script}"
QEMU_COMBINED="${6:?qemu_combined_dir}"
WIC_PARTITION="${7:?wic_partition}"
DATA_DIR="${8:-}"
EMCONFIG="${9:-}"
shift 9 || true

resolve_wic_partition() {
  local qemu="$1" spec="$2"
  local base="${spec%%:*}" suffix="${spec#*:}"
  if [ -e "${qemu}/${base}" ]; then
    echo "${spec}"
    return 0
  fi
  base="$(pick_first_glob "${qemu}" "*.rootfs.wic.ufs")" \
    || { log_error "no *.rootfs.wic.ufs under ${qemu}"; exit 1; }
  echo "${base}:${suffix}"
}

wic_cp_one() {
  local src="$1" dest="$2"
  [ -e "${src}" ] || { log_warn "skip missing: ${src}"; return 0; }
  wic cp --sector-size=4096 "${src}" "${dest}"
  log_info "wic copied $(basename "${src}")"
}

deploy_xclbin_aliases() {
  local src="$1" dest="$2" names="" name tmpdir="" seen=""
  [ -f "${src}" ] || return 0

  names="$(basename "${src}" .xclbin) ${XCLBIN_LOOKUP_NAMES:-}"
  tmpdir="$(mktemp -d)"
  trap 'rm -rf "${tmpdir}"' RETURN
  for name in ${names}; do
    [ -n "${name}" ] || continue
    [[ "${name}" =~ ^[A-Za-z0-9_]+$ ]] || continue
    case " ${seen} " in *" ${name} "*) continue ;; esac
    seen="${seen} ${name}"
    cp -f "${src}" "${tmpdir}/${name}.xclbin"
    wic_cp_one "${tmpdir}/${name}.xclbin" "${dest}"
  done
}

WIC_PARTITION="$(resolve_wic_partition "${QEMU_COMBINED}" "${WIC_PARTITION}")"
WIC_DEST="${QEMU_COMBINED}/${WIC_PARTITION}"

source_edf_yocto_sdk

for f in "${RUN_SCRIPT}" "${HOST_EXE}" "${DTBO}" "${PDI}"; do
  wic_cp_one "${f}" "${WIC_DEST}"
done
deploy_xclbin_aliases "${XCLBIN}" "${WIC_DEST}"
[ -n "${EMCONFIG}" ] && wic_cp_one "${EMCONFIG}" "${WIC_DEST}"

if [ -n "${DATA_DIR}" ] && [ -d "${DATA_DIR}" ]; then
  for item in "${DATA_DIR}"/*; do
    [ -e "${item}" ] && wic_cp_one "${item}" "${WIC_DEST}"
  done
fi
for item in "$@"; do
  wic_cp_one "${item}" "${WIC_DEST}"
done

if [ "${LINUX_EDF_INSTALL_VPP_BOOT:-0}" = 1 ]; then
  install_vpp_boot_on_ospi "${QEMU_COMBINED}" \
    "${EDF_BOOT_BIN:-${EDF_BOOT_DEFAULT}}" \
    "${QEMU_OSPI_BIN:-${EDF_OSPI_DEFAULT}}"
fi

log_info "edf_wic_deploy completed"
