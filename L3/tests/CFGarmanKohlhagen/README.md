# Closed Form Garman Kohlhagen Test

This test shows how to utilize the Garman Kohlhagen Model.

This model is the Black Scholes model with the risk free rate replaced by the domestic rate and the dividend yield replaced with the foreign rate.


# Setup Environment

    source <install path>/Vitis/2019.2/settings64.sh

    source /opt/xilinx/xrt/setup.sh


# Build the Xilinx Fintech L3 Library

    cd L3/src

    source env.sh or source env.csh

    make

# Build the matching GK kernel

    cd L2/tests/GarmanKohlhagenEngine

    make xclbin TARGET=sw_emu DEVICE=xilinx_u200_xdma_201920_1

# Build the Host code and run the executable
    cd L3/tests/hcf

    make run TARGET=sw_emu DEVICE=xilinx_u200_xdma_201920_1


*A symbolic link to the L2 kernel will be used when running the example, note if an error is displayed that the kernel does not exist refer to 'Build the matching HCF kernel' to build*

