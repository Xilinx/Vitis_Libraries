# Monte-Carlo European Engine
This is a benchmark of MC (Monte-Carlo) European Engine using the Xilinx Vitis environment to compare with QuantLib.  It supports software and hardware emulation as well as running the hardware accelerator on the Alveo U250.


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

            <host application> <mode> <xclbin>

For example example to run a prebuilt software emulation output (assuming the standard build directories):

    ./bin_xilinx_u250_xdma_201830_1/test.exe -xclbin xclbin_xilinx_u250_xdma_201830_1_sw_emu/kernel_mc.xclbin

Assuming an Alveo U250 card with the XRT configured, the hardware build is run as follows:

    ./bin_xilinx_u250_xdma_201830_1/test.exe -xclbin xclbin_xilinx_u250_xdma_201830_1_hw/kernel_mc.xclbin

## Example Output
for the testbench, process it via the engine and compare to the expected result, displaying the case difference. For example:

    ----------------------MC(European) Engine-----------------
    loop_nm = 1000
    Found Platform
    Platform Name: Xilinx
    Found Device=xilinx_u250_xdma_201830_1
    INFO: Importing kernel_mc.xclbin
    Loading: 'kernel_mc.xclbin'
    kernel has been created
    FPGA execution time: 0.273633s
    option number: 20480
    opt/sec: 74844.8
    Expected value: 3.833452
    FPGA result:
    			Kernel1 0 - 3.85024			Kernel 1 - 3.8436 			Kernel 2 - 3.85006 			Kernel 3 - 3.85304
    Execution time 454551



## Timing Performance

The timing performance of the MCEuropeanEngine is shown in the table below, where timesteps is 1, requiredSamples is 16383, and FPGA frequency is 250MHz.
The execution time is the average of 1000 runs.

| platform                | Execution time (cold run) | Execution time (warm run) |
| ----------------------- |---------------------------|---------------------------|
| QuantLib 1.15 on CentOS | 20.155  ms                |   20.155 ms               |
| FinTech on U250         | 0.053 ms                  |   0.01325ms               |  
| Accelaration Ratio      | 380X                      |   1521X                   |



##  Resource Utilization

The hardware resources are listed in the following table (vivado 18.3 report).

| BRAM | URAM | DSP  | FF      | LUT    |
| ---- | ---- | ---- | ------- | ------ |
| 196  | 0    | 6376 | 1504833 | 936288 |

