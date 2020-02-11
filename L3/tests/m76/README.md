# Merton Jump Diffusion Test

This test shows how to utilize the Merton Jump Diffusion Solution Model


# Setup Environment

    source <install path>/Vitis/2019.2/settings64.sh

    source /opt/xilinx/xrt/setup.sh


## Build the Xilinx Fintech L3 Library

    cd L3/src

    source env.sh or source env.csh

    make

## Build the matching M76 kernel

    cd L2/tests/M76Engine

    make xclbin TARGET=sw_emu DEVICE=xilinx_u200_xdma_201920_1

# Build the Host code and run the executable

    cd L3/tests/m76

    make run TARGET=sw_emu DEVICE=xilinx_u200_xdma_201920_1


*A symbolic link to the L2 kernel will be used when running the example, note if an error is displayed that the kernel does not exist refer to 'Build the matching M76 kernel' to build*


