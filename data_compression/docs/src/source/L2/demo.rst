.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Vitis, Library, Data Compression, AMD, FPGA OpenCL Kernels, LZ4 Demo, Snappy Demo, ZLIB Demo, GZip Demo, ZSTD Demo
   :description: This section provides various application demos
   :xlnxdocumentclass: Document
   :xlnxdocumenttypes: Tutorials

=====
Demos
=====

Demo examples for **lz4**, **snappy**, **lz4_streaming**, **zlib**, **gzip**, and **zstd** kernels are available in the ``L2/demos/`` directory.

.. toctree::
   :maxdepth: 1
   :caption: List of Demos

   AMD GZip Compression and Decompression <gzip.rst>
   AMD LZ4 Compression and Decompression <lz4.rst>
   AMD LZ4-Streaming Compression and Decompression <lz4_streaming.rst>
   AMD Snappy Compression and Decompression <snappy.rst>
   AMD Snappy-Streaming Compression and Decompression <snappy_streaming.rst>

.. note::
   Execute the following commands before building any of the examples:

.. code-block:: bash
      
   $ source <Vitis_Installed_Path>/installs/lin64/Vitis/2022.2/settings64.sh
   $ source <Vitis_Installed_Path>/xbb/xrt/packages/setup.sh

Build Instructions
------------------

Execute the following commands to compile and test run this example:

.. code-block:: bash
      
   $ make run TARGET=hw_emu

Variable ``TARGET`` can take the following values:

  - **hw_emu**: Hardware emulation.
  
  - **hw**: Run on actual hardware.

By default, the target device is set as AMD Alveo™ U200. To target a different device, use the  ``PLATFORM`` argument. For example:

.. code-block:: bash

    make run TARGET=hw_emu PLATFORM=<new_device.xpfm>

.. note::
   Build instructions explained in this section are common for all the applications. The generated executable names might differ.
