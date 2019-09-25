
# Closed Form Black Scholes Example

This example show how to utilize the Closed Form Black Scholes Model.


# Setup Environment

source /opt/xilinx/xrt/setup.csh

source /*path to xf_fintech*/L3/src/env.csh


# Build Xilinx Fintech Library

cd  /*path to xf_fintech*/L3/src

**make all**


# Build Instuctions

To build the command line executable (mc_example) from this directory

**make all**

> Note this requires the xilinx fintech library to already to built


# Run Instuctions

Copy the prebuilt kernel files from /*path to xf_fintech*/L2/tests/CFBlackScholes/ to this directory

**bs_kernel.xclbin**

To run the command line exe and generate the interpolated NPV

**make run**
