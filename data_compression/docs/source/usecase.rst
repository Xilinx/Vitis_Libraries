.. CompressionLib_Docs documentation master file, created by
   sphinx-quickstart on Thu Jun 20 14:04:09 2019.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

.. meta::
   :keywords: Vitis, Library, Data Compression, Xilinx, L1, L2, L3, Overlay, OpenCL Kernel, FPGA Kernel, HLS Kernel
   :description: Typlical usecases of Vitis Data Compression Library
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _use_case:

Typical Use Cases
=================

This library, in its current state, can be used for acceleration of data compression applications in two ways:

+-----------------------------+--------------------------------------------------------------------------------+
| Acceleration Scope          | Developer's Usage of Compression Library                                       |
+=============================+================================================================================+
| Individual Components       | Write a custom kernel with modules from the library.                           |
+-----------------------------+--------------------------------------------------------------------------------+
| Compression-Decompression   | Use a complete compression or decompression kernel (eg. lz4, snappy)           |
+-----------------------------+--------------------------------------------------------------------------------+


L1 module contains several primitive components which can be used in different algorithm kernels. Developers who only need these primitives to build their own kernels may refer :ref:`l1_user_guide`.

L2 module contains pre-designed compress/decompress kernels for various data compression algorithms. Developers can directly use these kernels in their design and may refer :ref:`l2_user_guide`.


Note

L3 Overlay is still under active development now. Stay tuned!

