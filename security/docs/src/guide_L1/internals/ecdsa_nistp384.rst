.. 
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
  
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
  
       http://www.apache.org/licenses/LICENSE-2.0
  
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

.. meta::
   :keywords: Vitis, Security, Library, CBC, mode
   :description: The Cipher Block Chaining (CBC) mode is a typical block cipher mode of operation using block cipher algorithm.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials



**************
ECDSA nistp384
**************

.. toctree::
   :maxdepth: 1

Overview
========

Elliptic Curve Digital Signature Algorithm (ECDSA) is a variant of the Digital Signature Algorithm (DSA) which is based on elliptic curve cryptography(ECC). 
The elliptic curve nistp256 we used to sign and verify signature is specified with a set of parameters defined in `Digital Signature Standard (DSS)
 <https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.186-4.pdf>`_.

Implementation on FPGA
======================

Signing (point multiplication kG)
---------------------------------
* Use a precompute table of multiples of curve base point G.
* Use NAF notation for k.
* Optimized modular multiplicaion based on prime p.
* Point addition/doubling calculated in Jacobian coordinate.

Verification (point multiplication aG+bP)
-----------------------------------------
* Use a precompute table of multiples of curve base point G.
* Use NAF notation for a.
* Optimized modular multiplicaion based on prime p.
* Point addition/doubling calculated in Jacobian coordinate.
