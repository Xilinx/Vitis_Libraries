## Level 1: HLS Functions and Modules

The Level 1 APIs are presented as HLS C++ classes and functions.

This level of API is mainly provide for hardware-savvy HLS developers.The API description and design details of these modules can be found in xfOpenCV User Guide.

'examples' folder contains the testbench and accel C++ files that demonstrate the call of xfOpenCV functions with xf::Mat interfaces.

'include' folder contains the definitions of all the functions in various hpp files

'tests' folder has sub-folders named according to the function and the configuration it would run. Each individual folder has Makefiles and config files that would perform CSIM, CSYNTH, CO-SIM etc., of a given function.

'standalone_hls_axi_example' folder shows the example function that uses AXI interfaces to call xfOpenCV functions.

### Commands to run:

source < path-to-Vitis-installation-directory >/settings64.sh

source < part-to-XRT-installation-directory >/setenv.sh

export DEVICE=< path-to-platform-directory >/<platform>.xpfm

export MK_COMMON_DIR=< path-to-xfopencv-repo >

make run CSIM=1 CSYNTH=1 COSIM=0