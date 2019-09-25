# Heston Closed Form Call Test

This test shows how to utilize the Heston Closed Form Solution Model

# Setup Environment

source /opt/xilinx/xrt/setup.csh

source /*path to xf_fintech*/L3/src/env.csh


# Build Xilinx Fintech Library
cd  /*path to xf_fintech*/L3/src

**make all**


# Build Instuctions

To build the command line executable from this directory

**make all**

> Note this requires the xilinx fintech library to have already been built


# Run Instuctions
Copy the prebuilt kernel files to this directory

**hcf_hw_u250_float.xclbin**

To run the command line exe and generate the NPV

**make run**

