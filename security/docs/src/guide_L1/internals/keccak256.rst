.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Vitis, Security, Library, Keccak 256, Algorithm
   :description: Keccak-256 is a cryptographic hash function defined in The KECCAK SHA-3 submission-Version 3[submit in 2011]. 
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

****************
Keccak-256 Algorithms
****************

.. toctree::
   :maxdepth: 1

Overview
========

Keccak-256 is a cryptographic hash function defined in: `The KECCAK SHA-3 submission-Version 3 [submit in 2011] <https://keccak.team/files/Keccak-submission-3.pdf>`_.


Implementation on FPGA
======================

Refer to SHA-3 for an internal structure design.

Padding rule is the only difference between two algorithm implementations: use 0x01 in Keccak-256 and 0x06 in SHA-3.
