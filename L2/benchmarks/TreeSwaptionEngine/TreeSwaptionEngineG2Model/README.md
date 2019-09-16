# Tree Swaption G2 Model Engine

This is a benchmark of  Tree Swaption G2 Model Engine using the SDx environment to compare with QuantLib.  It supports software and hardware emulation as well as running the hardware accelerator on the Alveo U250.

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
----------------------Tree Bermudan (G2) Engine-----------------
timestep=50
Found Platform
Platform Name: Xilinx
Found Device=xilinx_u250_xdma_201830_1
INFO: Importing xclbin/TREE_u250_sw_emu.xclbin
Loading: 'xclbin/TREE_u250_sw_emu.xclbin'
kernel has been created
kernel start------
kernel end------
FPGA Execution time: 1.9ms
QuantLib 1.15 CPU Execution time: 258.0ms
NPV= 14.1398 ,diff/NPV= 2.59649e-11

```



## Timing Performance

The timing performance of the TreeSwaptionEngine+G2Model is shown in the table below, where FPGA frequency is 250MHz, and the QuantLib's default timesteps is 50.

| platform                | timesteps = 50 | timesteps = 100 |
| ----------------------- | -------------- | --------------- |
| QuantLib 1.15 on CentOS | 258.0 ms       | 2133.5 ms       |
| FinTech on U250         | 1.9 ms         | 14.5 ms         |
| Accelaration Ratio      | 135.7          | 147.1           |



##  Resource Utilization

The hardware resources are listed in the following table (vivado 18.3 report).

| BRAM | URAM | DSP  | FF     | LUT   |
| ---- | ---- | ---- | ------ | ----- |
| 18   | 136  | 625  | 139467 | 90205 |

