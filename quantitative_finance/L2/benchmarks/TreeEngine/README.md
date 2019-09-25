# Tree Hull-White Model Engine

This is a benchmark of  Tree Hull-White Model Engine that include Swaption Engine, Swap Engine, CapFloor Engine, and CallableBond Engine using the SDx environment to compare with QuantLib.  It supports software and hardware emulation as well as running the hardware accelerator on the Alveo U250.

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

The timing performance of the TreeEngine based on HWModel is shown in the table below, where FPGA frequency is 300MHz, and the QuantLib's default timesteps is 50.

| Engine       | Timesteps                   | 50   | 100  | 500   | 1000   |
| ------------ | --------------------------- | ---- | ---- | ----- | ------ |
|              | CPU Execution time(ms)      | 1.0  | 4.8  | 353.9 | 2493.5 |
| Swaption     | FPGA Execution time-HLS(ms) | 0.28 | 0.61 | 5.72  | 18.17  |
|              | Accelaration Ratio          | 3.5  | 7.8  | 61.8  | 137.2  |
|              | CPU Execution time(ms)      | 1.0  | 4.3  | 291.2 | 2056.5 |
| Swap         | FPGA Execution time-HLS(ms) | 0.28 | 0.61 | 5.61  | 18.16  |
|              | Accelaration Ratio          | 3.5  | 7.0  | 51.9  | 113.2  |
|              | CPU Execution time(ms)      | 0.7  | 3.4  | 217.6 | 1581.3 |
| CapFloor     | FPGA Execution time-HLS(ms) | 0.30 | 0.64 | 5.89  | 18.51  |
|              | Accelaration Ratio          | 2.3  | 5.3  | 36.9  | 85.4   |
|              | CPU Execution time(ms)      | 1.4  | 3.5  | 155.2 | 1142.0 |
| CallableBond | FPGA Execution time-HLS(ms) | 0.28 | 0.60 | 5.67  | 17.89  |
|              | Accelaration Ratio          | 5.0  | 5.8  | 27.3  | 63.8   |



##  Resource Utilization

The hardware resources are listed in the following table (vivado 18.3 report).

| Model        | BRAM | URAM | DSP  | FF    | LUT   |
| ------------ | ---- | ---- | ---- | ----- | ----- |
| Swaption     | 112  | 0    | 452  | 87469 | 67212 |
| Swap         | 104  | 0    | 408  | 84628 | 65744 |
| CapFloor     | 104  | 0    | 364  | 79489 | 64863 |
| CallableBond | 104  | 0    | 320  | 76577 | 62445 |

