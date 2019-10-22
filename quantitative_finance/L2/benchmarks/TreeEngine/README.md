# Tree Hull-White Model Engine

This is a benchmark of Tree Engine based on 6 rate models that include Swaption Engine, Swap Engine, CapFloor Engine, and CallableBond Engine using the Vitis environment to compare with QuantLib.  It supports software and hardware emulation as well as running the hardware accelerator on the Alveo U250.

## Prerequisites

- Alveo U250 installed and configured as per https://www.xilinx.com/products/boards-and-kits/alveo/u250.html#gettingStarted
- Xilinx runtime (XRT) installed
- Xilinx Vitis 2019.2 installed and configured



## Building

The demonstration application and kernel is built using a command line Makefile flow.

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
	make run TARGET=hw DEVICE=xilinx_u250_xdma_201830_2
```

The Makefile supports software emulation, hardware emulation and hardware targets ('sw_emu', 'hw_emu' and 'hw', respectively).  

In the case of the software and hardware emulations, the Makefile will build and launch the host code as part of the run.  These can be rerun manually using the following pattern:

```
        <host application> <xclbin>
```

For example example to run a prebuilt software emulation output (assuming the standard build directories):

```
./bin_xilinx_u250_xdma_201830_2/host.exe -xclbin xclbin_xilinx_u250_xdma_201830_2_hw_emu/scanTreeKernel.xclbin
```

Assuming an Alveo U250 card with the XRT configured, the hardware build is run as follows:

```
./bin_xilinx_u250_xdma_201830_2/host.exe -xclbin xclbin_xilinx_u250_xdma_201830_2_hw/scanTreeKernel.xclbin
```

## Example Output

The testbench of process it via the engine and compare to the expected result, displaying the worst case difference. For example, the following is the key information from the software emulation:

```
----------------------Tree Bermudan (HW) Engine-----------------
timestep=50
Found Platform
Platform Name: Xilinx
Found Device=xilinx_u250_xdma_201830_2
INFO: Importing xclbin_xilinx_u250_xdma_201830_2_hw/scanTreeKernel.xclbin
Loading: 'xclbin_xilinx_u250_xdma_201830_2_hw/scanTreeKernel.xclbin'
kernel has been created
kernel start------
kernel end------
FPGA Execution time: 0.28ms
NPV= 13.1903146433444 ,diff/NPV= -1.30631162395178e-14

```


