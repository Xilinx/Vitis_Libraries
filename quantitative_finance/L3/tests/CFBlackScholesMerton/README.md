
# Closed Form Black Scholes Merton Test

This test shows how to utilize the Closed Form Black Scholes Merton Model.

This model is the Black Scholes model with the addition of Dividend Yield.


# Setup Environment

source /opt/xilinx/xrt/setup.csh

source /*path to xf_fintech*/L3/src/env.csh


# Build Xilinx Fintech Library

cd  /*path to xf_fintech*/L3/src

**make all**


# Build Instuctions

To build the command line executable from this directory

**make all**

> Note this requires the xilinx fintech library to already to built


# Run Instuctions

Copy the prebuilt kernel files to this directory

**bsm_kernel.xclbin**


To run the command line exe and generate the interpolated NPV

**make run**

