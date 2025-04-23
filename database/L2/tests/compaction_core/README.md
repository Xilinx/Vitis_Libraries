# Vitis Tests for compaction acceleration Kernel

**This kernel targets Alveo U200, the makefile does not support other devices.**

To run the test, execute the following command:

```
source /opt/xilinx/Vitis/2022.2/settings64.sh
source /opt/xilinx/xrt/setup.sh
make run TARGET=hw_emu PLATFORM=/path/to/<u200>/xpfm
```

`TARGET` can also be `hw`.
