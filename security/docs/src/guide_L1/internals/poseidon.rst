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
Poseidon Hash
**************

.. toctree::
   :maxdepth: 1

Overview
========

Poseidon is a hash algorithm work natively with GF(p) objects. 
It’s a cryptographic hash function designed to work with integrity proof systems like SNARKs, STARKs, etc.
You can find detailed parameters and cryptographic analysis in `POSEIDON: A New Hash Function for Zero-Knowledge Proof Systems<https://eprint.iacr.org/2019/458.pdf>`_.

Implementation on FPGA
======================

* Templated design, support different full-rounds and partial-rounds implementation.
* Stream interface, for input message, round constant, MDS matrix and output digest.
* Works with GF(p) of which p is a 256 bits width prime number.
* Generate 256 bits digest result
