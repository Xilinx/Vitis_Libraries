# Vitis Tests for Kernels

This folder contains basic test for each kernel. They are meant to discover simple regression errors.

**These kernels have only been tested on Alveo U200 and U250, the makefile does not support other devices.**

To run the test, execute the following command:

```
source <install path>/HEAD/Vitis/settings64.sh
source /opt/xilinx/xrt/setup.sh
export PLATFORM_REPO_PATHS=/opt/xilinx/platforms
make run TARGET=hw_emu PLATFORM=xilinx_u200_gen3x16_xdma_2_202110_1
```

`TARGET` can also be `hw`.
