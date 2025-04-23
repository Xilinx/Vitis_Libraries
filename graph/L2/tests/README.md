# Vitis Tests for Kernels

This folder contains basic test for each of Graph kernels. They are meant to discover simple regression errors.

**These kernels have only been tested on Alveo U200 and U250, the makefile does not support other devices.**

To run the test, execute the following command:

```
source <install path>/Vitis/2022.1/settings64.sh
source /opt/xilinx/xrt/setup.sh
export PLATFORM_REPO_PATHS=/opt/xilinx/platforms
make run TARGET=hw_emu PLATFORM=xilinx_u250_gen3x16_xdma_3_1_202020_1.xpfm
```

`TARGET` can also be `hw`.
