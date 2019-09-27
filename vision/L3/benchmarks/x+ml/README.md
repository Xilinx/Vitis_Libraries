## ML + X Benchmark

This example shows how various Vitis Vision funtions can be used to accelerate preprocessing of input images before feeding them to a Deep Neural Network (DNN) accelerator.

This specific application shows how pre-processing for Googlenet_v1 can be accelerated which involves resizing the input image to 224 x 224 size followed by mean subtraction.

**Performance:**

This pipeline is integrated with [xDNN](https://www.xilinx.com/support/documentation/white_papers/wp504-accel-dnns.pdf "xDNN whitepaper") accelerator and [MLsuite](https://github.com/Xilinx/ml-suite "ml-suite") to run Googlenet_v1 inference on [Alveo-U200](https://www.xilinx.com/products/boards-and-kits/alveo/u200.html "U200") accelerator card and achieved 11 % speed up compared to software pre-processing.

**Overall Performance (Images/sec)**

with software pre-processing             : 125 images/sec

with hardware accelerated pre-processing : 140 images/sec

### Commands to run:

source < path-to-Vitis-installation-directory >/settings64.sh

source < part-to-XRT-installation-directory >/setenv.sh

export DEVICE=< path-to-platform-directory >/< platform >.xpfm

**For PCIe devices:**

make host xclbin TARGET=< sw_emu|hw_emu|hw >

make run TARGET=< sw_emu|hw_emu|hw >

**For embedded devices:**

export SYSROOT=< path-to-platform-sysroot >

make host xclbin TARGET=hw BOARD=Zynq 

make run TARGET=< hw >

copy the image.ub, xclbins and executable to an SDCARD and run on the board.