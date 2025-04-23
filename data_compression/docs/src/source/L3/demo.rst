.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Vitis, Library, Data Compression, AMD, FPGA OpenCL Kernels, LZ4 Demo, ZLIB Demo
   :description: This section provides various application demos

=====
Demos
=====

This page describes the integration of various modules from L1 and L2 levels in combination with software APIs to derive an end application that can be directly deployed or creation of a shared library that can be integrated with external applications.

Demo examples for **Gzip** and **Lz4** applications are available in the ``L3/demos/`` directory.

.. toctree::
   :maxdepth: 1
   :caption: List of Demos

   LZ4 Application <lz4_app.rst>
   GZip Application <gzip_app.rst>

Environment Setup
=================

Execute the following commands to set up the AMD Vitis™ environment for building the application. These instructions are applicable for all the demos under this category.

.. code-block:: bash

    $source <Vitis_Installation_Path>/installs/lin64/Vitis/2022.2/settings64.csh
    $source <Vitis_Installation_Path>/xbb/xrt/packages/setup.sh

Build Instructions
------------------

Execute the following command to compile and test run this application:

.. code-block:: bash
   
   $ make run TARGET=hw_emu

Variable ``TARGET`` can take the following values:

	- **hw_emu**: Hardware emulation.
	
	- **hw**: Run on actual hardware.

Build instructions are common for all the applications. The generated executable might differ.
