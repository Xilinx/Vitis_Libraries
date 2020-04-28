# Monte-Carlo American Engine
This is a benchmark of MC (Monte-Carlo) American Engine using the Xilinx Vitis environment to compare with QuantLib.  It supports software and hardware emulation as well as running the hardware accelerator on the Alveo U250.


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

    ./bin_xilinx_u250_xdma_201830_1/host.exe -xclbin xclbin_xilinx_u250_xdma_201830_1_sw_emu/MCAE_k.xclbin

Assuming an Alveo U250 card with the XRT configured, the hardware build is run as follows:

    ./bin_xilinx_u250_xdma_201830_1/host.exe -xclbin xclbin_xilinx_u250_xdma_201830_1_hw/MCAE_k.xclbin

## Example Output
for the testbench, process it via the engine and compare to the expected result, displaying the case difference. For example:

    ----------------------MC(American) Engine-----------------
    data_size is 409600
    Found Platform
    Platform Name: Xilinx
    Found Device=xilinx_u250_xdma_201830_1
    INFO: Importing MCAE_k.xclbin
    Loading: 'MCAE_k.xclbin'
    kernel has been created
    kernel start------
    kernel end------
    Execution time 8333us
    output = 3.97936
    PASSED!!! the output is confidential!
    



## Timing Performance

The timing performance of the MCAmericanEngine is shown in the table below, where timesteps is 100, requiredSamples is 24576, calibSamples is 4096, and FPGA frequency is 300MHz.

| platform                |    Execution time (cold run)     |       Execution time (warm run)        |
| ----------------------- | -------------------------------- |----------------------------------------|
| QuantLib 1.15 on CentOS | 1156.5ms                         | 1156.5m                                |
| FinTech on U250         | 5.87ms                           | 1.96ms                                 |
| Accelaration Ratio      | 197X                             | 590X                                   |



##  Resource Utilization

The hardware resources are listed in the following table (vivado 18.3 report).

kernels                   | BRAM | URAM | DSP  | FF     | LUT    |
--------------------------| ---- | ---- | ---- | ------ | ------ |
MCAmericanEnginePreSamples| 43   | 0    | 416  | 185169 | 120756 |
------------------------- | ---- | -----|------| -------| -------|
MCAmericanEngineCalibrate | 68   | 0    | 462  | 267405 | 181793 |
--------------------------| -----| -----|------|--------|--------|
MCAmericanEnginePriceing  | 71   | 0    | 911  | 368839 | 251370 |

