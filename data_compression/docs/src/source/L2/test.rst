.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Vitis, Library, Data Compression, AMD, FPGA OpenCL Kernels, LZ4 Test, Snappy Test, ZLIB Test, GZip Test, ZSTD Test
   :description: This section provides various application tests
   :xlnxdocumentclass: Document
   :xlnxdocumenttypes: Tutorials

=====
Tests
=====

Tests examples for **lz4**, **snappy**, **lz4_streaming**, **snappy_streaming**, **zlib**, **gzip**, and **zstd** kernels are available in the ``L2/tests/`` directory.

.. toctree::
   :maxdepth: 1
   :caption: List of Tests

   AMD LZ4 Streaming Compression <lz4_compress_streaming.rst>
   AMD Snappy Compression <snappy_compress.rst>
   AMD GZip Compression <gzipc_block_mm.rst>
   AMD GZip Streaming Compression <gzipc.rst>
   AMD GZip Streaming 16KB Compression <gzipc_16KB.rst>
   AMD GZip Streaming 8KB Compression <gzipc_8KB.rst>
   AMD GZip Static Streaming Compression <gzipc_static.rst>
   AMD Zlib Streaming Compression <zlibc.rst>
   AMD Zlib Streaming 16KB Compression <zlibc_16KB.rst>
   AMD Zlib Streaming 8KB Compression <zlibc_8KB.rst>
   AMD Zlib Streaming Static Compression <zlibc_static.rst>
   AMD ZSTD Compression <zstd_quadcore_compress.rst>
   AMD LZ4 Streaming Decompression <lz4_dec_streaming_parallelByte8.rst>
   AMD Snappy Streaming Decompression <snappy_dec_streaming_parallelByte8.rst>
   AMD ZSTD Decompression <zstdd_32KB.rst>
   
.. note::
Execute the following commands before building any of the examples:

.. code-block:: bash
      
   $ source <Vitis_Installed_Path>/installs/lin64/Vitis/2022.2/settings64.sh
   $ source <Vitis_Installed_Path>/xbb/xrt/packages/setup.sh

Build Instructions
------------------

Execute the following commands to compile and test run this example:

.. code-block:: bash
      
   $ make run TARGET=sw_emu

Variable ``TARGET`` can take the following values:

  - **sw_emu**: Software emulation.
  
  - **hw_emu**: Hardware emulation.
  
  - **hw** : Run on actual hardware.

By default, the target device is set as AMD Alveo™ U200. To target a different device, use the  ``PLATFORM`` argument. For example:

.. code-block:: bash

    make run TARGET=sw_emu PLATFORM=<new_device.xpfm>

.. note::
   Build instructions explained in this section are common for all the applications. The generated executable names might differ.
