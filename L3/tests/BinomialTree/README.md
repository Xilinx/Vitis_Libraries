
# Binomial Tree Example

This example show how to utilize the BinomialTree Model

# Setup Environment

source /opt/xilinx/xrt/setup.csh

source /*path to xf_fintech*/L3/src/env.csh

# Build Xilinx Fintech Library

cd  /*path to xf_fintech*/L3/src

**make all**

# Build Instuctions

To build the command line executable (binomial tree example) from this directory

**make all**

> Note this requires the xilinx fintech library to have already been built


# Run Instuctions

Copy the prebuilt kernel files from /*path to xf_fintech*/L2/tests/BinomialTreeEngine/ to this directory

BinomialTree_hw_u250_float_pe8.xclbin


To run the command line exe and generate the NPV

**make run**

