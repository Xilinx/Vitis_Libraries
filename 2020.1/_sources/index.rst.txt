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

========================
Vitis DSP Library
========================

Vitis DSP library provides implementation of different L1/L2/L3 primitives for digital signal processing. 
Current version only provides implementation of Discrete Fourier Transform using Fast 
Fourier Transform algorithm for acceleration on Xilinx FPGAs. The Library is planned
to provide three types of implementations namely L1 primitives, L2 kernels and L3 software APIs. Those 
implementations are organized in their corresponding directories L1, L2 and L3.
The L1 primitives can be leveraged by developers working on harware design 
implementation or designing hardware kernels for acceleration. L1 primitives are partitcularly
suitable for hardware savy designers. The L2 kernels are HLS-based predesigned kernels 
that can be directly used for FPGA acceleration of different applications on integration with
Xilinx Runtime (XRT). The L3 provides software APIs in C, C++ and Python which 
allows software developers to offload FFT calculation to FPGAs for acceleration. Before
an FGPA can perform the FFT computation. The FPGA needs to be configured with a particular image
called an Overlay.

Since all the kernel code is developed with the permissive Apache 2.0 license,
advanced users can easily tailor, optimize or combine them for their own needs.
Demos and usage examples of different level implementations are also provided
for reference. 

**Note**: Current release of the Vitis FFT only provides L1 primitives.

.. toctree::
   :caption: Library Overview
   :maxdepth: 1

   overview.rst
   release.rst
 
.. toctree::
   :caption: L1 User Guide
   :maxdepth: 1

   user_guide/L1.rst
   user_guide/L1_2dfft.rst
