## Level 3: Pipeline Applications

This directory contains whole applications formed by stitching a pipeline of Vitis Vision functions. The host code shows how to call this multiple functions in OpenCL.

'examples' folder contains the OpenCL host code file and a C++ accel file that demonstrate the call of Vitis Vision functions to build for Vitis.

'build' folder inside 'examples' folder has makefile that would build the default configuration of the function.

'tests' folder has sub-folders named according to the function and the configuration it would run. Each individual folder has Makefiles and config files that would perform software emulation, hardware emulation and hardware build of the corresponding function in examples folder, based on the 'Board' the user selects.

'benchmarks' directory has applications ready to build that give out their performance comparison against other architectures.

### Commands to run:

source < path-to-Vitis-installation-directory >/settings64.sh

source < part-to-XRT-installation-directory >/setenv.sh

export DEVICE=< path-to-platform-directory >/<platform>.xpfm

**For PCIe devices:**

make host xclbin TARGET=< sw_emu|hw_emu|hw >

make run TARGET=< sw_emu|hw_emu|hw >

**For embedded devices:**

export SYSROOT=< path-to-platform-sysroot >

make host xclbin TARGET=hw BOARD=Zynq 

make run TARGET=< hw >

copy the image.ub, xclbins and executable to an SDCARD and run on the board.