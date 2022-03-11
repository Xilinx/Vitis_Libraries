.. GenomicsLib_Docs documentation master file, created by
   sphinx-quickstart on Thu Jan 13 14:04:09 2022.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

.. meta::
   :keywords: Vitis, Library, Genomics, Xilinx, L1, L2, L3, Overlay, OpenCL Kernel, FPGA Kernel, HLS Kernel
   :description: Typlical usecases of Vitis Genomics Library

.. _use_case:

Typical Use Cases
=================

The Vitis Genomics library, in its current state, can be used for acceleration of genomics applications in two ways:

+-----------------------------+--------------------------------------------------------------------------------+
| Acceleration Scope          | Developer's Usage of Genomics Library                                          |
+=============================+================================================================================+
| Individual Components       | Write a custom kernel with modules from the library.                           |
+-----------------------------+--------------------------------------------------------------------------------+
| Genomic                     | Use a complete genomics kernel (eg. Smithwaterman)                             |
+-----------------------------+--------------------------------------------------------------------------------+


L1 module contains several primitive components which can be used in different algorithm kernels. For information on primitives to build your own kernels, see :ref:`l1_user_guide`.

L2 module contains pre-designed genomics kernels for various genomics algorithms. You can directly use these kernels in your design. For more information, see :ref:`l2_user_guide`.


.. note::
L3 Overlay is currently under active development.

