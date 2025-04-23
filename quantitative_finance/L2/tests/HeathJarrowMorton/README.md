## Heath-Jarrow-Morton framework (HJM) Demonstration
This is a demonstration of the HJM framework built using the Vitis environment. It supports hardware emulation as well as running the hardware accelerator on supported Alveo cards.

The demonstration run the kernel to price a ZeroCouponBond from an initial set of historical data. It will
analyse the data, extract the volatilities and drift from it, and perform a MonteCarlo simulation with the
calculated data.

## Prerequisites

- Alveo U200 installed and configured as per https://www.xilinx.com/products/boards-and-kits/alveo/u200.html#gettingStarted
- Xilinx runtime (XRT) installed
- Xilinx Vitis 2020.1 installed and configured

## Building the demonstration
The kernel and host application is built using a command line Makefile flow.

### Step 1 :
Setup the build environment using the Vitis and XRT scripts:

            source <install path>/Vitis/2020.1/settings64.sh
            source /opt/xilinx/xrt/setup.sh

### Step 2 :
Call the Makefile passing in the intended target and platform (.xpfm). For example:

            make all TARGET=hw_emu PLATFORM=xilinx_u250_xdma_201830_2

 The Makefile supports hardware emulation and hardware targets ('hw_emu' and 'hw', respectively). The host application (hjm_test) is written to the root of this demo folder, and the xclbin is delivered into the xclbin* folder with a name identifying the card and target.  For example the U200 hardware emulation build produces:

            ./xclbin_xilinx_u250_xdma_201830_2_hw_emu/hjm_kernel.xclbin'

The xclbin is passed as a parameter to the host code along with the historical data file and the number of MC paths to generate.
The hardware emulation can be run as follows, a smaller number of parameters should be used as an RTL simulation is used under-the-hood:

            export XCL_EMULATION_MODE hw_emu
            ./bin_xilinx_u250_xdma_201830_2/hjm_test.exe -x xclbin_xilinx_u250_xdma_201830_2_hw_emu/hjm_kernel.xclbin -d ./hist_data.csv -p 10

Assuming an Alveo U200 card with the XRT configured the hardware build is run in the same way.  Here a much large number of parameters should be used to fully exercise the device:

            unset XCL_EMULATION_MODE
            ./bin_xilinx_u250_xdma_201830_2/hjm_test.exe -x xclbin_xilinx_u250_xdma_201830_2_hw/hjm_kernel.xclbin -d ./hist_data.csv -p 1000
