
# Monte Carlo Example

This example show how to utilize BOTH the MC-European and MC-American models within the same executable


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

Copy the prebuilt kernel files from /*path to xf_fintech*/L2/tests/MCEuropeanEngine/ & /*path to xf_fintech*/L2/tests/MCAmericanEngine/ to this directory

**mc_euro_k.xclbin**
**MCAE_k.xclbin**


To run the command line exe and generate the results

**make run**
