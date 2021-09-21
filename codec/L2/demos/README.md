# Benchmark Test Overview

Here are benchmarks of the Vitis Codec Library using the Vitis environment and comparing with CPU. It supports software and hardware emulation as well as running hardware accelerators on the Alveo U200.

## Prerequisites

### Vitis Codec Library
- Alveo U200 installed and configured as per [Alveo U200 Data Center Accelerator Card](https://www.xilinx.com/products/boards-and-kits/alveo/u200.html#gettingStarted)
- Xilinx runtime (XRT) installed
- Xilinx Vitis 2021.2 installed and configured

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

These codec benchmarks can be downloaded from [vitis libraries](https://github.com/Xilinx/Vitis_Libraries.git) ``master`` branch.

```
   git clone https://github.com/Xilinx/Vitis_Libraries.git
   cd Vitis_Libraries
   git checkout master
   cd codec 
```

- ### Setup environment

Specifying the corresponding Vitis, XRT, and path to the platform repository by running following commands.

```
   source <intstall_path>/installs/lin64/Vitis/2021.2/settings64.sh
   source /opt/xilinx/xrt/setup.sh
   export PLATFORM_REPO_PATHS=/opt/xilinx/platforms
```
