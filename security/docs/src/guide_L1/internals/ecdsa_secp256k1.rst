.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Vitis, Security, Library, CBC, mode
   :description: The Cipher Block Chaining (CBC) mode is a typical block cipher mode of operation using block cipher algorithm.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials



***************
ECDSA secp256k1
***************

.. toctree::
   :maxdepth: 1

Overview
========

Elliptic Curve Digital Signature Algorithm (ECDSA) is a variant of the Digital Signature Algorithm (DSA), which is based on an elliptic curve cryptography(ECC). 
The elliptic curve secp256k1 is used to sign and verify signature is specified with a set of parameters defined in `Standards for Efficient
Cryptography 2 (SEC 2) 2.4.1 <https://www.secg.org/sec2-v2.pdf>`_.

Implementation on FPGA
======================

Signing (point multiplication kG)
---------------------------------
* Use a precompute table of multiples of curve base point G.
* Use NAF notation for k.
* Optimized modular multiplication based on prime p .
* Point addition/doubling calculated in Jacobian coordinate.
* Time used to generate signature is around 0.2ms.

Verification (point multiplication aG+bP)
-----------------------------------------
* Use a precompute table of multiples of curve base point G.
* Use NAF notation for a.
* Optimized modular multiplication based on prime p.
* Point addition/doubling calculated in Jacobian coordinate.
* Time used to verify signature is around 0.9ms.

