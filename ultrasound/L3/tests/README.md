## Ultrasound Library - Level 3 (L3)

### Setup Environment

Starting with 2026.2, embedded platforms use the Yocto-based EDF common image. Install steps are in *Getting Started with Versal AI Edge Gen2 with Vitis Unified IDE* ([XD098](https://docs.amd.com/r/en-US/Vitis-Tutorials-Getting-Started/Getting-Started-with-Versal-AI-Edge-Gen2-with-Vitis-Unified-IDE)).

```bash
#!/bin/bash
# setup vitis_26.2 env
source <Vitis_install_path>/2026.2/Vitis/settings64.sh
source /opt/xilinx/xrt/setup.sh
export PLATFORM_REPO_PATHS=<path to platforms>
export PLATFORM=vck190_base
# starting with 2026.2, set up your YOCTO_ARTIFACTS PATH (Yocto-based EDF common image)
export YOCTO_ARTIFACTS=<path to design>/yocto_artifacts
# for 2026.1 and earlier releases, set up your SYSROOT, ROOTFS and K_IMAGE PATH
export SYSROOT=<path to platforms>/sw/versal/xilinx-versal/sysroots/cortexa72-cortexa53-amd-linux
export ROOTFS=<path to platforms>/sw/versal/xilinx-versal/rootfs.ext4
export K_IMAGE=<path to platforms>/sw/versal/xilinx-versal/Image
```

### Run a L3 Example
```bash
#!/bin/bash
cd L3/tests/scanline     # scanline is an example case. Please change directory to any other cases in L3/tests if interested.
make help                # show available make command
make run TARGET=x86sim   # run aie x86sim flow
make run TARGET=aiesim   # run aie aiesim flow
make run TRAGET=hw_emu   # run vitis hw_emu flow
make all TARGET=hw       # build the entire program for hw_run
```

## License

Licensed using the [Apache 2.0 license](https://www.apache.org/licenses/LICENSE-2.0).

    Copyright (C) 2019-2022, Xilinx, Inc.
    Copyright (C) 2022-2026, Advanced Micro Devices, Inc.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
