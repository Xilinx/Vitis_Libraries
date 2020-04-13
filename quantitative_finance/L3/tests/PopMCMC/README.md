
# Population Monte Carlo Markov Chain Example

This example show how to utilize the Population Monte Carlo Markov Chain Model

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
Build the matching PopMCMC Kernel

    cd L2/tests/PopMCMC

    make xclbin TARGET=sw_emu DEVICE=xilinx_u200_xdma_201920_1


### Step 4 :
Build host code & run executable

    cd L3/tests/PopMCMC

    make run TARGET=sw_emu DEVICE=xilinx_u200_xdma_201920_1

Note : The output will populated into the file pop_mcmc_output.csv

*A symbolic link to the L2 kernel will be used when running the example, note if an error is displayed that the kernel does not exist refer to step 3 to build*

