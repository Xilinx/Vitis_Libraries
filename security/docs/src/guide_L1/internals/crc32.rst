.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Vitis, Security, Library, CRC
   :description: A cyclic redundancy check (CRC) is an error-detecting code commonly which encode messages by adding a fixed-length check value, for the purpose of error detection.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


******************
CRC32
******************

.. toctree::
   :maxdepth: 1

Overview
========

A cyclic redundancy check (CRC) is an error-detecting code commonly which encode messages by adding a fixed-length check value, for the purpose of error detection. CRC32 is a 32-bit CRC code. More details are found in `Wiki CRC`_.

.. _`Wiki CRC`: https://en.wikipedia.org/wiki/Cyclic_redundancy_check

Implementation on FPGA
======================

For the CRC32 design, it can be specified as:

- Width : 32
- Poly  : 0xEDB88320 (crc32) or 0x82F63B78 (crc32c)
- Init  : 0xFFFFFFFF
- Way   : Lookup table

For the :math:`table` and the CRC32 implementation, please check out `Ref`_.

.. _'Ref`: https://create.stephan-brumme.com/crc32/#slicing-by-16-overview

For more information, check out source code.
