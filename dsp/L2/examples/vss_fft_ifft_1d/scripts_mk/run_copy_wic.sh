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

XCLBIN_DIR=$1
HOST_DIR=$2
DTBO_DIR=$3
PDI_DIR=$4
RUN_SCRIPT=$5
QEMU_COMBINED=$6
WIC_PARTITION=$7
DATA_PATH=$8
shift 8
DATA_FILE_LIST=("$@")

unset LD_LIBRARY_PATH
source ${YOCTO_ARTIFACTS}/amd-cortexa78-mali-common_meta-edf-app-sdk/sdk/environment-setup-cortexa72-cortexa53-amd-linux
echo "sourced"

wic cp --sector-size=4096 ${RUN_SCRIPT}                               ${QEMU_COMBINED}/${WIC_PARTITION}
echo "copied ${RUN_SCRIPT}"                                 
wic cp --sector-size=4096 ${HOST_DIR}                                 ${QEMU_COMBINED}/${WIC_PARTITION}
echo "copied ${HOST_DIR}"                                                
wic cp --sector-size=4096 ${XCLBIN_DIR}                               ${QEMU_COMBINED}/${WIC_PARTITION}
echo "copied ${XCLBIN_DIR}"                                                  
wic cp --sector-size=4096 ${DTBO_DIR}                                 ${QEMU_COMBINED}/${WIC_PARTITION}
echo "copied ${DTBO_DIR}"
wic cp --sector-size=4096 ${PDI_DIR}                                  ${QEMU_COMBINED}/${WIC_PARTITION}
echo "copied ${PDI_DIR}"

if [ -n "$DATA_PATH" ] && [ -d "$DATA_PATH" ]; then
  for f in "$DATA_PATH"/*; do
    [ -e "$f" ] || continue
      wic cp --sector-size=4096 "$f" "${QEMU_COMBINED}/${WIC_PARTITION}"
      echo "copied $(basename "$f")"
  done
else
  echo "DATA_PATH is empty, skip copy"
fi

if [ ${#DATA_FILE_LIST[@]} -gt 0 ]; then
  for f in "${DATA_FILE_LIST[@]}"; do
    if [ -f "$f" ]; then
      wic cp --sector-size=4096 "$f" "${QEMU_COMBINED}/${WIC_PARTITION}"
      echo "copied $(basename "$f")"
    else
      echo "Warning: DATA_FILE $f not found, skip"
    fi
  done
else
  echo "DATA_FILE list is empty, skip copy"
fi
