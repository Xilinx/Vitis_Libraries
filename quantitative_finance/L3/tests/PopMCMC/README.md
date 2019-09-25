
# Binomial Tree Example

This example show how to utilize the Population Monte Carlo Markov Chain Model


# Setup Environment

source /opt/xilinx/xrt/setup.csh

source /*path to xf_fintech*/L3/src/env.csh


# Build Xilinx Fintech Library

cd  /*path to xf_fintech*/L3/src

**make all**


# Build Instuctions

To build the command line executable (markov chain example) from this directory

**make clean all**

> Note this requires the xilinx fintech library to have already been built


# Run Instuctions

Copy the prebuilt kernel files from /*path to xf_fintech*/L2/tests/PopMCMC/ to this directory

mcmc_kernel.xclbin


To run the command line exe and generate the result

**make run**

The output will populated into the file pop_mcmc_output.csv

