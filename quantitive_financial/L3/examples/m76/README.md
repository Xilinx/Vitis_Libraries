# Merton Jump Diffusion Example

This example show how to utilize the Merton Jump Diffusion Solution Model


# Setup Environment

source /opt/xilinx/xrt/setup.csh

source /*path to xf_fintech*/L3/src/env.csh


# Build and exectue the example

## 1. Build the kernel

> cd /*path to xf_fintech*/L2/demos/HCFEngine

> make all TARGET=hw

## 2. Build the Xilinx Fintech Library

> cd /*path to xf_fintech*/L3/src

> make all

## 3. Build the example application

> cd /*path to xf_fintech*/L3/examples/m76

> make all


## 4. Run the example
Copy the prebuilt kernel files to this directory

> cp /*path to xf_fintech*/L2/demos/HCFEngine/xclbin/m76_hw_u200_float.xclbin .

To run the command line exe and generate the NPV

> make run
