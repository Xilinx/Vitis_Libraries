# Tree Swaption Engine

This is a benchmark of  Tree Swaption Engine that include Vasicek model, Hull-White model, Black-Karasinski model, Cox-Ingersoll-Ross model, Extended Cox-Ingersoll-Ross model using the SDx environment to compare with QuantLib.  It supports software and hardware emulation as well as running the hardware accelerator on the Alveo U250.

## Prerequisites

- Alveo U250 installed and configured as per https://www.xilinx.com/products/boards-and-kits/alveo/u250.html#gettingStarted
- Xilinx runtime (XRT) installed
- Xilinx SDx 2018.3 installed and configured



## Building

The demonstration application and kernel is built using a command line Makefile flow.

### Step 1 :

Setup the build environment using the SDx and XRT scripts:

```
        source env_sdx.sh
```

### Step 2 :

Call the Makefile. For example:

```
        make run_hw 
```

The Makefile supports software emulation, hardware emulation and hardware targets ('sw_emu', 'hw_emu' and 'hw', respectively).  

To avoid errors, several example bash scripts are provided to build the default sofware emulation, hardware emulation and hardware targets.

```
        source run_sw_emu.sh
        source run_hw_emu.sh
        source run_hw.sh
```

In the case of the software and hardware emulations, the Makefile will build and launch the host code as part of the run.  These can be rerun manually using the following pattern:

```
        <host application> <mode> <xclbin>
```

For example example to run a prebuilt software emulation output (assuming the standard build directories):

```
./bin/host.exe -mode fpga -xclbin xclbin/TREE_u250_sw_emu.xclbin
```

Assuming an Alveo U250 card with the XRT configured, the hardware build is run as follows:

```
./bin/host.exe -mode fpga -xclbin xclbin/TREE_u250_hw.xclbin
```

## Example Output

The testbench of process it via the engine and compare to the expected result, displaying the worst case difference. For example, the following is from the software emulation:

```
----------------------Tree Bermudan (HullWhite) Engine-----------------
timestep=50
Found Platform
Platform Name: Xilinx
Found Device=xilinx_u250_xdma_201830_1
INFO: Importing xclbin/TREE_u250_sw_emu.xclbin
Loading: 'xclbin/TREE_u250_sw_emu.xclbin'
kernel has been created
kernel start------
kernel end------
FPGA Execution time: 0.28ms
QuantLib 1.15 CPU Execution time: 1.0ms
NPV= 13.1903146433444 ,diff/NPV= -1.30631162395178e-14

```



## Timing Performance

The timing performance of the TreeSwaptionEngine is shown in the table below, where FPGA frequency is 300MHz, and the QuantLib's default timesteps is 50.

| Model     | Timesteps                   | 50   | 100  | 500   | 1000   |
| --------- | --------------------------- | ---- | ---- | ----- | ------ |
|           | CPU Execution time(ms)      | 1.0  | 4.8  | 353.9 | 2493.5 |
| HWModel   | FPGA Execution time-HLS(ms) | 0.28 | 0.61 | 5.72  | 18.17  |
|           | Accelaration Ratio          | 3.5  | 7.8  | 61.8  | 137.2  |
|           | CPU Execution time(ms)      | 1.9  | 8.6  | 438.2 | 2813.1 |
| BKModel   | FPGA Execution time-HLS(ms) | 0.72 | 1.53 | 11.93 | 34.21  |
|           | Accelaration Ratio          | 2.6  | 5.6  | 36.7  | 82.2   |
|           | CPU Execution time(ms)      | 0.5  | 1.4  | 26.6  | 100.7  |
| CIRModel  | FPGA Execution time-HLS(ms) | 0.16 | 0.31 | 2.22  | 6.18   |
|           | Accelaration Ratio          | 3.1  | 4.5  | 11.9  | 16.2   |
|           | CPU Execution time(ms)      | 1.1  | 5.5  | 439.5 | 3322.5 |
| ECIRModel | FPGA Execution time-HLS(ms) | 0.72 | 1.36 | 10.17 | 28.47  |
|           | Accelaration Ratio          | 1.5  | 4.0  | 43.2  | 116.7  |
|           | CPU Execution time(ms)      | 0.5  | 1.8  | 40.1  | 161.7  |
| VModel    | FPGA Execution time-HLS(ms) | 0.14 | 0.29 | 2.42  | 7.42   |
|           | Accelaration Ratio          | 3.5  | 6.2  | 16.5  | 21.7   |



##  Resource Utilization

The hardware resources are listed in the following table (vivado 18.3 report).

| Model     | BRAM | URAM | DSP  | FF     | LUT   |
| --------- | ---- | ---- | ---- | ------ | ----- |
| HWModel   | 112  | 0    | 452  | 87469  | 67212 |
| BKModel   | 116  | 0    | 495  | 99209  | 82034 |
| CIRModel  | 104  | 0    | 417  | 82910  | 51160 |
| ECIRModel | 116  | 0    | 442  | 102802 | 81395 |
| VModel    | 104  | 0    | 377  | 74551  | 48322 |

