.. 
   Copyright 2019 Xilinx, Inc.
  
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

Please refer to SHA-3 for internal structure design.

Padding rule is the only difference between two algorithm implementations: use 0x01 in Keccak-256 and 0x06 in SHA-3.
