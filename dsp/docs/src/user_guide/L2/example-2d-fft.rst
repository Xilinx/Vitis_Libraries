.. 
   Copyright © 2019–2024 Advanced Micro Devices, Inc
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

*********************
FFT across AIE and PL
*********************

This example design illustrates that the implementation of an algorithm can be split over the AIE and programmable logic for high performance. The AIE is well suited for the regular and compute intensive part of the algorithm and the PL offers an efficient way of implementing complex routing needed in hardware.
Internal details of the implementation are documented in `FFT tutorial <https://github.com/Xilinx/Vitis-Tutorials/tree/2023.2/AI_Engine_Development/AIE/Design_Tutorials/12-IFFT64K-2D>`_.

The following environment variables need to be set for the combined hardware emulation of the AIE and PL system: 
* SYSROOT - < path-to-platform-sysroot >
* XILINX_XRT - < path-to-platform-xrt > 

To run the complete system on hardware or hardware emulation, you can use the Makefile in the directory with TARGET=hw_build or TARGET=hw_emu respectively.