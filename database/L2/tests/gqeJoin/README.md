# Vitis Tests for gqeJoin Kernel

**This kernel has been tested on Alveo U280 and Alveo U50, the makefile does not support other devices.**

To run the test, execute the following command:

```
source /opt/xilinx/Vitis/2022.2/settings64.sh
source /opt/xilinx/xrt/setup.sh
make run TARGET=hw_emu PLATFORM=/path/to/<u280|u50>/xpfm
```

`TARGET` can also be `hw`.
