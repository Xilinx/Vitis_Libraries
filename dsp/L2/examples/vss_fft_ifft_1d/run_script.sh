#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
#
export LD_LIBRARY_PATH=/mnt:/tmp:
export XCL_EMULATION_MODE=hw_emu
export XILINX_VITIS=/mnt
export XILINX_XRT=/usr
if [ -f platform_desc.txt  ]; then
        cp platform_desc.txt /etc/xocl.txt
fi
./host.elf
return_code=$?
if [ $return_code -ne 0 ]; then
        echo "ERROR: TEST FAILED, RC=$return_code"
else
        echo "INFO: TEST PASSED, RC=0"
fi
echo "INFO: Embedded host run completed."
exit $return_code
