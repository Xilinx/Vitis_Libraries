# Benchmark Test Overview

Here are benchmarks of the Vitis Graph Library using the Vitis environment and comparing with Spark (v3.0.0) GraphX. It supports software and hardware emulation as well as running hardware accelerators on the Alveo U250.

## Prerequisites

### Vitis Graph Library
- Alveo U250 installed and configured as per https://www.xilinx.com/products/boards-and-kits/alveo/u250.html#gettingStarted
- Xilinx runtime (XRT) installed
- Xilinx Vitis 2019.2 installed and configured

### Spark
- Spark 3.0.0 installed and configured
- Spark running on platform with Intel(R) Xeon(R) CPU E5-2690 v4 @2.600GHz, 56 Threads (2 Sockets, 14 Core(s) per socket, 2 Thread(s) per core)

### Datasets
- Datasets from https://sparse.tamu.edu/
- Format requirement: compressed sparse row (CSR) or compressed sparse column (CSC).

## Building

Here, TriangleCount is taken as an example to indicate how to build the application and kernel with the command line Makefile flow.

### Step 1 :

Setup the build environment using the Vitis and XRT scripts:

```
        source <install path>/Vitis/2019.2/settings64.sh
	source /opt/xilinx/xrt/setup.sh
```

### Step 2 :

Call the Makefile. For example:

```
        export PLATFORM_REPO_PATHS=/opt/xilinx/platforms
	make run TARGET=sw_emu DEVICE=xilinx_u250_xdma_201830_2
```

The Makefile supports various build target including software emulation, hardware emulation and hardware ('sw_emu', 'hw_emu' and 'hw', respectively). 

In the case of the software and hardware emulations, the Makefile will build and launch the host code as part of the run.  These can be rerun manually using the following pattern:

```
        <host application> <xclbin> <argv>
```

For example, to run a prebuilt software emulation output (assuming the standard build directories):

```
./bin_xilinx_u250_xdma_201830_2/host.exe -xclbin xclbin_xilinx_u250_xdma_201830_2_sw_emu/TC_Kernel.xclbin -o data/csr_offsets.txt -i data/csr_columns.txt
```

Assuming XRT with an Alveo U250 card has been configured, the hardware build is run as follows:

```
./bin_xilinx_u250_xdma_201830_2/host.exe -xclbin xclbin_xilinx_u250_xdma_201830_2_hw/TC_Kernel.xclbin -o data/csr_offsets.txt -i data/csr_columns.txt
```

## Example Output

The testbench will compute the input data via the kernel and will compare with the expected result. For example, the following is the key information from the software emulation:

```
---------------------Triangle Count-----------------
Found Platform
Platform Name: Xilinx
Found Device=xilinx_u250_xdma_201830_2
INFO: Importing xclbin_xilinx_u250_xdma_201830_2_sw_emu/TC_kernel.xclbin
Loading: 'xclbin_xilinx_u250_xdma_201830_2_sw_emu/TC_kernel.xclbin'
kernel start------
vertexNum=10
edgeNum=20
offset Arr2Strm
row Arr2Strm
offset Arr2Strm2
row Arr2Strm2
coreControlImpl
coreControlImpl2
row1CopyImpl
row2Impl
mergeImpl
tcAccUnit
triangle count = 11
kernel end------
Execution time 3352.76ms
Write DDR Execution time 3348.48 ms
Kernel Execution time 4.01216 ms
Read DDR Execution time 0.106196 ms
Total Execution time 3352.69 ms
INFO: case pass!

```


