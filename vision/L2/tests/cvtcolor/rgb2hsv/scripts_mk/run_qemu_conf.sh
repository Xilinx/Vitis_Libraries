#!/bin/bash

unset LD_LIBRARY_PATH
source ${XILINX_VITIS}/data/emulation/qemu/comp/qemu/environment-setup-x86_64-petalinux-linux
echo "sourced"
./run.sh
