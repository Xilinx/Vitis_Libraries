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

=====================
XF Database Library
=====================

XF Database Library is an open-sourced Vitis library written in C++ for
accelerating database applications in a variety of use cases.
It now covers two levels of acceleration: the module level and the pre-defined kernel level,
and will evolve to offer the third level as pure software APIs working with pre-defined hardware overlays.

* At module level, it provides optimized hardware implementation of most common relational database execution plan steps,
  like hash-join and aggregation.
* In kernel level, the post-bitstream-programmable kernel can be used to map a sequence of execution plan steps,
  without having to compile FPGA binaries for each query.
* The upcoming software API level will wrap the details of offloading acceleration with prebuilt binary (overlay)
  and allow users to accelerate supported database tasks on Alveo cards without hardware development.

Since all the kernel code is developed in HLS C++ with the permissive Apache 2.0 license,
advanced users can easily tailor, optimize or combine with property logic at any levels.
Demo/examples of different database acceleration approach are also provided with the library for easy on-boarding.

.. toctree::
   :caption: Library Overview
   :maxdepth: 1

   overview.rst
   release.rst

.. toctree::
   :caption: User Guide
   :maxdepth: 2

   usecase.rst
   guide/L1.rst
   gqe_guide/L2.rst
   gqe_guide/L3.rst

.. toctree::
   :caption: Benchmark Result
   :maxdepth: 1

   benchmark/tpc_h.rst

