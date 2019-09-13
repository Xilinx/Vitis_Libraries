.. CompressionLib_Docs documentation master file, created by
   sphinx-quickstart on Thu Jun 20 14:04:09 2019.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.


XF Compression Library
======================

XF Compression Library is an open-sourced VITIS library written in C++ for accelerating data compression applications in a variety of use cases. It now covers two levels of acceleration: the module level and the pre-defined kernel level, and will evolve to offer the third level as pure software APIs working with pre-defined hardware overlays.

- At module level, it provides optimized hardware implementation of common sub-systems used in kernels of multiple compression algorithm, like stream.
- In kernel level, the post-bitstream-programmable kernel can be used to perform a series of compressions or decompressions, without having to re-compile FPGA binaries to process multiple files.
- The upcoming software API level will wrap the details of offloading acceleration with prebuilt binary (overlay) and allow users to accelerate supported database tasks on Alveo cards without hardware development.

Since all the kernel code is developed in HLS C++ with the permissive Apache 2.0 license, advanced users can easily tailor, optimize or combine with property logic at any levels. Demo/examples of different accelerated compression algorithms are also provided with the library for easy on-boarding.


.. toctree::
   :maxdepth: 1
   :caption: Library Overview
   
   source/overview.rst

.. toctree::
	:maxdepth: 2
	:caption: User Guide

	source/usecase.rst
	source/L1/L1.rst
	source/L2/L2.rst
..	source/L3/L3.rst

.. toctree::
	:caption: Benchmark Results
	:maxdepth: 1
	
	source/results.rst

