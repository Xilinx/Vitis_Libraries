# CSV Scanner Vitis System Compiler Project

This project is trying to give a showcase for how to integrate the CSV scanner hardware accelerator with System Compiler tool to decompress a compresssed CSV file using Gzip algorithm, hash and/or filter for specific column(s).

## Contents

* The multithreaded test code can be found in the current directory, and the detailed example usage can be found in the [HTML doc](https://xilinx.github.io/Vitis_Libraries/data_analytics/2022.1/index.html).
* The implementation of the L3 APIs are resided in `xf_DataAnalytics/L3/include/sw/xf_data_analytics/gunzip_csv` and `xf_DataAnalytics/L3/src/sw/gunzip_csv`, to simplify the users integration efforts, we packed the Vitis System Compiler application layer in our L3 APIs to implement a task queue that is used to take the acceleration request from thread and emit the result to the corresponding thread.
* For detailed hardware accelerator implementation, please kindly find it at `xf_DataAnalytics/L2/include/hw/xf_data_analytics/dataframe`.

## How to Use

For command-line developers the following settings are required before running any case in this library:

```console
source /opt/xilinx/Vitis/2022.1/settings64.sh
source /opt/xilinx/xrt/setup.sh
export PLATFORM_REPO_PATHS=/opt/xilinx/platforms
```

For `csh` users, please look for corresponding scripts with `.csh` suffix and adjust the variable setting command accordingly.

The `PLATFORM_REPO_PATHS` environment variable points to directories containing platforms.

```console
# build and run one of the following using U.2. platform
make run TARGET=sw_emu DEVICE=/path/to/xilinx_u2_gen3x4_xdma_gc_2_202110_1.xpfm

# delete generated files
make cleanall
```

Here, `TARGET` decides the FPGA binary type
- `sw_emu` is for software emulation
- `hw_emu` is for hardware emulation
- `hw` is for deployment on physical card. (Compilation to hardware binary often takes hours.)

