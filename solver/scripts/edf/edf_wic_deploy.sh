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
# Deploy application artifacts into the WIC rootfs for Linux EDF hw_emu.
#
# Usage:
#   edf_wic_deploy.sh <xclbin> <host_exe> <dtbo> <pdi> <run_script> \
#       <qemu_combined_dir> <data_dir> <emconfig> [data_files...]
#
# <dtbo> and <pdi> are optional. Platform parameters come from the EDF_*
# environment, see edf_hw_emu_lib.sh.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=edf_hw_emu_lib.sh
source "${SCRIPT_DIR}/edf_hw_emu_lib.sh"

XCLBIN="${1:?xclbin}"
HOST_EXE="${2:?host_exe}"
DTBO="${3:-}"
PDI="${4:-}"
RUN_SCRIPT="${5:?run_script}"
QEMU_COMBINED="${6:?qemu_combined_dir}"
DATA_DIR="${7:-}"
EMCONFIG="${8:-}"
shift 8 || true

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

WIC_IMAGE="$(resolve_wic_image "${QEMU_COMBINED}")"
# the fat32 application partition, mounted at /media in the guest
WIC_DEST="${QEMU_COMBINED}/${WIC_IMAGE}:${EDF_WIC_APP_PART:-2}"

source_edf_yocto_sdk

for f in "${RUN_SCRIPT}" "${HOST_EXE}"; do
  [ -n "${f}" ] && wic_cp_one "${f}" "${WIC_DEST}"
done

# the overlay is platform dependent, its absence is not an error
if [ -n "${DTBO}" ] && [ -f "${DTBO}" ]; then
  wic_cp_one "${DTBO}" "${WIC_DEST}"
  [ -n "${PDI}" ] && [ -f "${PDI}" ] && wic_cp_one "${PDI}" "${WIC_DEST}"
elif [ -n "${DTBO}" ]; then
  log_info "no PL overlay for this platform, guest does not load the PL at run time"
fi
deploy_xclbin_aliases "${XCLBIN}" "${WIC_DEST}"
[ -n "${EMCONFIG}" ] && wic_cp_one "${EMCONFIG}" "${WIC_DEST}"

# DATA_DIR may name several directories, copy each whole like --package.sd_dir
for data_dir in ${DATA_DIR}; do
  wic_cp_one "${data_dir%/}" "${WIC_DEST}"
done
for item in "$@"; do
  wic_cp_one "${item}" "${WIC_DEST}"
done

if [ "${LINUX_EDF_INSTALL_VPP_BOOT:-1}" = 1 ]; then
  install_vpp_boot "${QEMU_COMBINED}"
fi

log_info "edf_wic_deploy completed"
