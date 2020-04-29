
# Setup Environment

source /opt/xilinx/xrt/setup.csh

source /*path to xf_fintech*/L3/src/env.csh

# Build Xilinx Fintech Library

cd  /*path to xf_fintech*/L3/src

**make clean all**


# Build Instuctions

To build the command line executable (hjm_cl) from this directory

**make clean all**

> Note this requires the xilinx fintech library to already to built


# Run Instuctions

Copy the prebuilt kernel files from /*path to xf_fintech*/L2/tests/HeathJarrowMorton/ to this directory

**hjm_kernel_u200.xclbin**

To run the command line exe and generate the Zero Coupon Bond price run

>output/hjm_cl.exe -d data.csv [-p no_paths] [-s sim_years] [-m zcb_maturity]

or with default arguments

**make run**
