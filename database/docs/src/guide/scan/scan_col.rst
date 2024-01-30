.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: scan, scanCol
   :description: Describes the structure and execution of the dynamic evaluation module.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _guide-scan:

********************************************************
Internals of Scan
********************************************************

.. toctree::
   :hidden:
   :maxdepth: 1

This document describes the structure and execution of dynamic evaluation module, implemented as a ``scanCol`` function.

Scan is a function to transfer data from an external DDR/HBM port to an internal HLS stream. As you known, a stream based data interface can be easily processed in a FPGA, so it provides a bridge to across from the external memory to the FPGA. There are three versions of ``scanCol`` in ``xf_database`` now: 

Version1: Defined in ``L1/include/hw/xf_database/scan_col.hpp``. In this head file, six kinds of ``scanCol`` are provided to cope with 1-6 column number in the DDR/HBM. Each column has its own DDR/HBM port as input, and the data is scanned into 1-6 stream as output. An example to scan one-column's data is provided into multiple-channel's output stream, and this logic is very easy to be redesigned for other dedicated purposes.

Version2: Defined in ``L1/include/hw/xf_database/scan_col_2.hpp``. Unlike version1, a data structure is designed in this version, and it provides the row number of a column inside the memory header, so there is no need for ``Nrow`` in the API of version2. It is suggested to use this version for cases in which the row number of a column is unknown or not decided before calling kernels.

Version3: Defined in ``L1/include/hw/xf_database/scan_cmp_str_col.hpp``. This version provides an internal logic scan and compare string. The output is a boolean value to indicate whether the input string is equal to the constant string. It is more cost efficient to process the boolean result than directly using the original string on the FPGA.