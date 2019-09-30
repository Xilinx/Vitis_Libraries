## Level 1: HLS Functions and Modules

The Level 1 APIs are presented as HLS C++ classes and functions.

This level of API is mainly provide for hardware-savvy HLS developers.The API description and design details of these modules can be found in Vitis Vision User Guide.

'examples' folder contains the testbench and accel C++ files that demonstrate the call of Vitis Vision functions with xf::Mat interfaces.

'build' folder inside 'examples' folder has makefile that would build the default configuration of the function.

'include' folder contains the definitions of all the functions in various hpp files

'tests' folder has sub-folders named according to the function and the configuration it would run. Each individual folder has Makefiles and config files that would perform C-Simulation, Synthesis, Co-Simulation etc., of the corresponding function in the example folder using standalone Vivado HLS.


### Commands to run:

source < path-to-Vitis-installation-directory >/settings64.sh

source < part-to-XRT-installation-directory >/setenv.sh

export DEVICE=< path-to-platform-directory >/< platform >.xpfm

make run CSIM=1 CSYNTH=1 COSIM=0

Note : Please read "Getting started with HLS" section of [Vitis Vision documentation] (https://xilinx.github.io/Vitis_Libraries/vision/) for special cases, constraints and other full details.