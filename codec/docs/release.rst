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

.. _release_note:

Release Note
============

.. toctree::
   :hidden:
   :maxdepth: 1

Codec Library is an open-sourced library written in C/C++ for accelerating image coding, decoding and related processing algorithms. It now covers a level of acceleration: the module level(L1) and the pre-defined kernel level(L2).

2022.1
----

The 2022.1 release provides a range of algorithms, includes:

1. JPEG decoding: one L2 API is provided for accelerating entire JPEG decoding process, which supports the ‘Sequential DCT-based mode’ of ISO/IEC 10918-1 standard. It can process 1 Huffman token and create up to 8 DCT coefficients within one cycle. It is also an easy-to-use decoder as it can directly parse the JPEG file header without help of software functions. In addition, L1 API is provided for Huffman decoding.
