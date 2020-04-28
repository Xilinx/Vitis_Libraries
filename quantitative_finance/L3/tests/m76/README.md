# Merton Jump Diffusion Test

This test shows how to utilize the Merton Jump Diffusion Solution Model


# Setup Environment

source /opt/xilinx/xrt/setup.csh

source /*path to xf_fintech*/L3/src/env.csh


## Build the Xilinx Fintech Library

cd /*path to xf_fintech*/L3/src

**make all**

## Build Instructions

To build the command line executable from this directory

**make all**

> Note this requires the xilinx fintech library to already to built


# Run Instuctions

Copy the prebuilt kernel files to this directory

**m76_hw_u250_float.xclbin**


To run the command line exe and generate the interpolated NPV

**make run**



