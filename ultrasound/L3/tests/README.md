## Ultrasound Library - Level 3 (L3)

### Setup Environment
```bash
#!/bin/bash
# setup vitis_24.2 env
source <Vitis_install_path>/Vitis/2024.2/settings64.sh
source /opt/xilinx/xrt/setup.sh
export PLATFORM_REPO_PATHS=<path to platforms>
export PLATFORM=xilinx_vck190_base_202420_1
# set up your SYSROOT, ROOTFS and K_IMAGE PATH
export SYSROOT=<path to platforms>/sw/versal/xilinx-versal-common-v2024.2/sysroots/aarch64-xilinx-linux/
export ROOTFS=<path to platforms>/sw/versal/xilinx-versal-common-v2024.2/rootfs.ext4
export K_IMAGE=<path to platforms>/sw/versal/xilinx-versal-common-v2024.2/Image
```

### Run a L3 Example
```bash
#!/bin/bash
cd L3/tests/scanline     # scanline is an example case. Please change directory to any other cases in L3/tests if interested.
make help                # show available make command
make run TARGET=x86sim   # run aie x86sim flow
make run TARGET=aiesim   # run aie aiesim flow
make run TARGET=sw_emu   # run vitis sw_emu flow
make run TRAGET=hw_emu   # run vitis hw_emu flow
make all TARGET=hw       # build the entire program for hw_run
```

## License

Licensed using the [Apache 2.0 license](https://www.apache.org/licenses/LICENSE-2.0).

    Copyright (C) 2019-2022, Xilinx, Inc.
    Copyright (C) 2022-2024, Advanced Micro Devices, Inc.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
