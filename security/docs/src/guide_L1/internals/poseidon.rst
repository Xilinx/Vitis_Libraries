.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Vitis, Security, Library, CBC, mode
   :description: The Cipher Block Chaining (CBC) mode is a typical block cipher mode of operation using block cipher algorithm.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials



**************
Poseidon Hash
**************

.. toctree::
   :maxdepth: 1

Overview
========

Poseidon is a hash algorithm work natively with GF(p) objects. 
It is a cryptographic hash function designed to work with integrity proof systems like SNARKs, STARKs, and so on.
You can find detailed parameters and cryptographic analysis in `POSEIDON: A New Hash Function for Zero-Knowledge Proof Systems<https://eprint.iacr.org/2019/458.pdf>`_.

Implementation on FPGA
======================

* Templated design, support different full-rounds and partial-rounds implementation.
* Stream interface, for input message, round constant, MDS matrix and output digest.
* Works with GF(p) of which p is a 256 bits width prime number.
* Generate 256 bits digest result
