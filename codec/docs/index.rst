.. 
   Copyright 2022 Xilinx, Inc.
  
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
  
       http://www.apache.org/licenses/LICENSE-2.0
  
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.


.. Project documentation master file, created by
   sphinx-quickstart on Tue Oct 30 18:39:21 2018.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Vitis Codec Library
==========================

Codec Library is an open-sourced library written in C/C++ accelerating image coding, decoding and related processing. The algorithms of JPEG decoding is accelerated


- For JPEG decoding, one L2 API is provided for accelerating entire JPEG decoding process, which supports the ‘Sequential DCT-based mode’ of ISO/IEC 10918-1 standard. It can process 1 Huffman token and create up to 8 DCT coefficients within one cycle. It is also an easy-to-use decoder as it can directly parse the JPEG file header without help of software functions. In addition, L1 API is provided for Huffman decoding. 


.. toctree::
   :caption: Library Overview
   :maxdepth: 1

   overview.rst
   release.rst

.. toctree::
   :caption: L1 User Guide
   :maxdepth: 3

   guide_L1/api.rst

.. toctree::
   :maxdepth: 2

   guide_L1/internals.rst

.. toctree::
   :caption: L2 User Guide
   :maxdepth: 3

   guide_L2/api.rst

.. toctree::
   :maxdepth: 2

   guide_L2/internals.rst

.. toctree::
   :caption: Benchmark 
   :maxdepth: 1 

   benchmark.rst

Indices and tables
------------------

* :ref:`genindex`
* :ref:`search`
