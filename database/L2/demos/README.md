# Accelerate TPC-H Queries with GQE Kernels

This demo shows TPC-H SQL query acceleration with GQE kernels.
For each query, scale factor 1 and 30 are supported.

As comparison, each query is also implemented in C++, and time of each execution step can be printed.
The data is pre-processed into numeric data arrays, for both CPU C++ and FPGA execution.

For more details of the kernel and test result, please refer to the HTML document.

```
# To build the xclbin files for tests
make run TARGET=<sw_emu|hw_emu|hw> DEVICE=u280-es1_xdma_201830_1

#To run a specific demo:
make run TARGET=<sw_emu|hw_emu|hw> DEVICE=u280-es1_xdma_201830_1 TB=<Q1|Q2|...> MODE=<FPGA|CPU>
```
