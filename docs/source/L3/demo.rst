
.. meta::
   :keywords: Vitis, Library, Data Compression, Xilinx, FPGA OpenCL Kernels, LZ4 Demo, ZLIB Demo
   :description: This section provides various application demos
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

=====
Demos
=====

This section presents integration of various modules from L1, L2 levels in
combination with software APIs to derive end application that can be directly
deployed or creation of shared library that can be integrated with external
applications.

Demo examples for Zlib and Lz4 applications are present in **L3/demos/**
directory.

.. toctree::
   :maxdepth: 1
   :caption: List of Demos

   lz4_app.rst
   zlib_app.rst

Environment Setup
=================

Instructions below setup Vitis environment for builid the application. These
instructions are applicable for all the demos under this category.


.. code-block:: bash

    $source <Vitis_Installation_Path>/installs/lin64/Vitis/2019.2/settings64.csh
    $source <Vitis_Installation_Path>/xbb/xrt/packages/setenv.sh

Build Instructions
------------------

To compile and test run this application execute the following commands:

.. code-block:: bash
   
   $ make run TARGET=sw_emu

Variable ``TARGET`` can take the following values:

	- **sw_emu**	: software emulation
	
	- **hw_emu**	: hardware emulation
	
	- **hw**	: run on actual hardware


Build instructions are common for all the application but generated executable
differ.


