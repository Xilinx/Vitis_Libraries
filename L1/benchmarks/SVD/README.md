# Singular Value Decomposition (SVD)
This is a benchmark of Singular Value Decomposition.  It supports software and hardware emulation as well as running the hardware accelerator on the Alveo U250.


## Prerequisites

- Alveo U250 installed and configured as per https://www.xilinx.com/products/boards-and-kits/alveo/u250.html#gettingStarted
- Xilinx runtime (XRT) installed
- Xilinx Vitis 2019.2 installed and configured

## Building
The demonstration application and kernel is built using a command line Makefile flow.

### Step 1 :
Setup the build environment using the Vitis and XRT scripts:

            source <install path>/Vitis/2019.2/settings64.sh
            source /opt/xilinx/xrt/setup.sh

### Step 2 :
Call the Makefile. For example:

            make run_hw 

The Makefile supports software emulation, hardware emulation and hardware targets ('sw_emu', 'hw_emu' and 'hw', respectively).  

In the case of the software and hardware emulations, the Makefile will build and launch the host code as part of the run.  These can be rerun manually using the following pattern:

            <host application> <xclbin>

For example example to run a prebuilt software emulation output (assuming the standard build directories):

    ./bin_xilinx_u250_xdma_201830_1/host.exe -xclbin xclbin_xilinx_u250_xdma_201830_1_sw_emu/kernel_svd_0.xclbin

Assuming an Alveo U250 card with the XRT configured, the hardware build is run as follows:

    ./bin_xilinx_u250_xdma_201830_1/host.exe -xclbin xclbin_xilinx_u250_xdma_201830_1_hw/kernel_svd_0.xclbin

## Example Output
for the testbench, process it via the kernel and compare to the expected result, displaying the case difference. For example:
    Found Platform
    Platform Name: Xilinx
    Found Device=xilinx_u250_xdma_201830_1
    INFO: Importing ./xclbin_xilinx_u250_xdma_201830_1_hw/kernel_svd_0.xclbin
    Loading: './xclbin_xilinx_u250_xdma_201830_1_hw/kernel_svd_0.xclbin'
    kernel has been created
    finished data transfer from h2d
    Kernel 0 done!
    kernel execution time : 22 us
    result correct
    
## Timing Performance

The timing performance of the 4x4 SVD is shown in the table below, where matrix size is 4 x 4, and FPGA frequency is 300MHz.

|  platform                         |    Execution time (cold run)     |      Execution time (warm run)        |
| --------------------------------- | -------------------------------- |---------------------------------------|
| MKL Intel(R) Xeon(R) E5-2690 v3   |   N/A                            |   8 us                                |
| FinTech on U250                   |   196 us                         |   22 us                               |
| Accelaration Ratio                |   N/A                            |   0.36X                               |



##  Resource Utilization

The hardware resources are listed in the following table (vivado 18.3 report without platform).

| primitive | BRAM | URAM | DSP  | FF     | LUT    |
| --------- | ---- | ---- | ---- | ------ | ------ |
|     SVD   |   9  |   0  | 126  |  46360 |  40313 |

