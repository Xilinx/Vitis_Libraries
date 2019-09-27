# Vitis Tests for gqeJoin Kernel

**This kernel has only been tested on Alveo U280, the makefile does not support other devices.**

To run the test, execute the following command:

```
source /opt/xilinx/Vitis/2019.2/settings64.sh
source /opt/xilinx/xrt/setup.sh
make run TARGET=sw_emu DEVICE=/path/to/u280/xpfm
```

`TARGET` can also be `hw_emu` or `hw`.
