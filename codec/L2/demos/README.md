# Benchmark Test Overview

Here are benchmarks of the Vitis Codec Library using the Vitis environment and comparing with CPU. It supports hardware emulation as well as running hardware accelerators on the Alveo U200.

## Prerequisites

### Vitis Codec Library
- Alveo U200 installed and configured as per [Alveo U200 Data Center Accelerator Card](https://www.xilinx.com/products/boards-and-kits/alveo/u200.html#gettingStarted)
- Xilinx runtime (XRT) installed
- Xilinx Vitis 2022.1 installed and configured

## Pictures

- Format requirement: the input is commonly used pictures that are listed in table 1.

Table 1 Pictures for benchmark

|    Pictures    |  Format  |    Size    |
|----------------|----------|------------|
|   android.jpg  |    420   |  960*1280  |
|   offset.jpg   |    422   |  5184*3456 |
|     hq.jpg     |    444   |  5760*3840 |
|   iphone.jpg   |    420   |  3264*2448 |
| lena_c_512.png |    444   |  512*512   |
| 1920x1080.png  |    444   |  1920*1080 |

## Building

Here, TriangleCount is taken as an example to indicate how to build the application and kernel with the command line Makefile flow.

- ### Download code

These codec benchmarks can be downloaded from [vitis libraries](https://github.com/Xilinx/Vitis_Libraries.git) ``main`` branch.

```
   git clone https://github.com/Xilinx/Vitis_Libraries.git
   cd Vitis_Libraries
   git checkout main
   cd codec 
```

- ### Setup environment

Specifying the corresponding Vitis, XRT, and path to the platform repository by running following commands.

```
   source <intstall_path>/installs/lin64/Vitis/2022.1/settings64.sh
   source /opt/xilinx/xrt/setup.sh
   export PLATFORM_REPO_PATHS=/opt/xilinx/platforms
```

## License

Licensed using the [Apache 2.0 license](https://www.apache.org/licenses/LICENSE-2.0).

    Copyright (C) 2019-2022, Xilinx, Inc.
    Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
    
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
    
        http://www.apache.org/licenses/LICENSE-2.0
    
    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
