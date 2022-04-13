
.. meta::
   :keywords: Vitis, Library, Genomics, Xilinx, FPGA OpenCL Kernels, PairHmm Demo, Smem Demo
   :description: This section provides various application demos

=====
Demos
=====

This page describes the integration of various modules from L1, L2 levels in
combination with software APIs to derive end application that can be directly
deployed or creation of shared library that can be integrated with external
applications.

Demo examples for **PairHMM** and **SMEM** applications are available in the ``L3/demos/`` directory.

.. toctree::
   :maxdepth: 1
   :caption: List of Demos

   pairhmm.rst
   smem.rst

Environment Setup
=================

Execute the following commands to setup the Vitis environment for building the application. These
instructions are applicable for all the demos under this category.


.. code-block:: bash

    $source <Vitis_Installation_Path>/installs/lin64/Vitis/2021.2/settings64.csh
    $source <Vitis_Installation_Path>/xbb/xrt/packages/setup.sh

Build Instructions
------------------

Execute the following command to compile and test run this application:

.. code-block:: bash
   
   $ make run TARGET=sw_emu

Variable ``TARGET`` can take the following values:

	- **sw_emu**	: software emulation
	
	- **hw_emu**	: hardware emulation
	
	- **hw**	: run on actual hardware


Build instructions are common for all the applications. The generated executable
may differ.


