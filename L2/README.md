## Level 2: Kernels and Engines

Level 2 contains the host-callable kernels and engines for various xfOpenCV functions.

'examples' folder contains the OpenCL host code file and a C++ accel file that demonstrate the call of xfOpenCV functions to build for Vitis.

'tests' folder has sub-folders named according to the function and the configuration it would run. Each individual folder has Makefiles and config files that would perform software emulation, hardware emulation and hardware build of the corresponding function in examples folder, based on the 'Board' the user selects.

### Commands to run:

source < path-to-Vitis-installation-directory >/settings64.sh

source < path-to-XRT-installation-directory >/setenv.sh

export DEVICE=< path-to-platform-directory >/<platform>.xpfm

**For PCIe devices:**

make host xclbin TARGET=< sw_emu|hw_emu|hw >

make run TARGET=< sw_emu|hw_emu|hw >

**For embedded devices:**

export SYSROOT=< path-to-platform-sysroot >

make host xclbin TARGET=hw BOARD=Zynq 

make run TARGET=< hw >

copy the image.ub, xclbins and executable to an SDCARD and run on the board.