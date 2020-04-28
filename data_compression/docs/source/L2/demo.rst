
.. meta::
   :keywords: Vitis, Library, Data Compression, Xilinx, FPGA OpenCL Kernels, LZ4 Demo, Snappy Demo, ZLIB Demo, GZip Demo
   :description: This section provides various application demos
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

=====
Demos
=====

Demo examples for lz4, snappy, lz4_streaming, zlib and gzip kernels are present in **L2/demos/** directory.

.. toctree::
   :maxdepth: 1
   :caption: List of Demos

   gzip.rst
   lz4.rst
   lz4_streaming.rst
   snappy.rst
   snappy_streaming.rst
   zlib.rst
   zlib_streaming.rst

Before building any of the examples, following commands need to be executed:

.. code-block:: bash
   
   $ source <Vitis_Installed_Path>/installs/lin64/Vitis/2019.2/settings64.sh
   $ source <Vitis_Installed_Path>/xbb/xrt/packages/setenv.sh

Build Instructions
------------------

To compile and test run this example execute the following commands:

.. code-block:: bash
   
   $ make run TARGET=sw_emu

Variable ``TARGET`` can take the following values:

	- **sw_emu**	: software emulation
	
	- **hw_emu**	: hardware emulation
	
	- **hw**	: run on actual hardware

By default the target device is set as Alveo U200. In order to target a different
device use "DEVICE" argument. Example below explains the same.

.. code-block:: bash

    make run TARGET=sw_emu DEVICE=<new_device.xpfm>

Build instructions explained in this section are common for all the
applications but the generated executable names differ.

