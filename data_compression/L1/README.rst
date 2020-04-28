==================
Primitive Overview
==================

The Level 1 APIs of Vitis Data Compression Library is presented as HLS
C++ modules. This Algorithm Library provides a range of primitives for implementing data compression in C++ for Vitis. Headers for these hardware APIs can be found in ``include`` directory of the package.

Stream-based Interface
``````````````````````
The interface of primitives in this library are mostly HLS streams, with the main input stream along with output stream throughout the dataflow.

The benefits of this interface are

- Within a HLS dataflow region, all primitives connected via HLS streams can work in parallel, and this is the key to FPGA acceleration.

This level of API is mainly provided for hardware-savvy developers. The
API description and design details of these modules can be found in L1
Module User Guide section of the `library
document <https://xilinx.github.io/Vitis_Libraries/data_compression/source/L1/L1.html>`__.
