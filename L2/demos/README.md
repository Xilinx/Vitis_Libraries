# Accelerate TPC-H Queries with GQE Kernels

This demo shows TPC-H SQL query acceleration with just one or two GQE xclbin files. Two scale factors, 1 and 30, are supported in this demo. For reference, each query is also implemented in single-thread C++ which prints time of each execution step on CPU.

For more details of the kernel and test result, please refer to the HTML document.

## Running the demo

Other than the standard `TARGET` and `DEVICE` variable, the following variables are used to specify the test:

* `MODE`: can be `CPU` or `FPGA`. Select `CPU` to run C++ implementation on host, and `FPGA` to use device.
* `SF`: can be `1` or `30`. The data will be automatically generated in `db_data` subfolder at first run using selected scale factor.
* `TB` can be `Q1` to `Q22`, except for `Q19` which is not supported yet.

```
# To build the xclbin files for tests
cd build_aggr_partition
make xclbin TARGET=<sw_emu|hw_emu|hw> DEVICE=/path/to/u280/xpfm
cd ..

cd build_join_partition
make xclbin TARGET=<sw_emu|hw_emu|hw> DEVICE=/path/to/u280/xpfm
cd ..

#To run a specific demo:
make run TARGET=<sw_emu|hw_emu|hw> TB=<Q1|Q2|...> MODE=<FPGA|CPU> SF=<1|30> DEVICE=/path/to/u280/xpfm
```
