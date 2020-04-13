
# Closed Form Black Scholes Merton Test

This test shows how to utilize the Closed Form Black Scholes Merton Model.

### Step 1 :
Setup the build environment using the Vitis and XRT scripts:

    source <install path>/Vitis/2019.2/settings64.sh

    source /opt/xilinx/xrt/setup.sh

### Step 2 :
Build the L3 Library

    cd  L3/src

    source env.sh or source env.csh

    make


### Step 3 :
Build the matching CFBlackScholesMerton Kernel

    cd L2/tests/CFBlackScholesMerton

    make xclbin TARGET=sw_emu DEVICE=xilinx_u200_xdma_201920_1


### Step 4 :
Build host code & run exeutable

    cd L3/tests/CFBlackScholesMerton

    make run TARGET=sw_emu DEVICE=xilinx_u200_xdma_201920_1


*A symbolic link to the L2 kernel will be used when running the example, note if an error is displayed that the kernel does not exist refer to step 3 to build*

