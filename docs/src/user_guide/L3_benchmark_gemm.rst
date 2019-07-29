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

.. _benchmark_gemm_l3:

======================
L3 API GEMM benchmark
======================

1. Intel® Math Kernel Library (MKL)
------------------------------------

1.1 Introduction
^^^^^^^^^^^^^^^^^
Intel® Math Kernel Library provides performance improvement of math functions, e.g. GEMM, when running with Intel processors. To compare with Xilinx's GEMX library, you can use our run-script (run_gemm_mkl.sh) to generate the data and performance benchmark.

1.2 Prerequisites
^^^^^^^^^^^^^^^^^^

**Intel® MKL**: Assume you have installed Intel® MKL, run the appropriate script to set up the environment variables (such as $MKLROOT).

.. code-block:: bash
 
  source <INTEL_MKL_INSTALL_DIR>/bin/mklvars.sh intel64
  
**NUMACTL**: The linux operating system provides a function, called numactl, that allows the control of scheduling or memory placement policy, which is essential to run parallel programs.

For Ubuntu (you only need to do it once),

.. code-block:: bash
 
  sudo apt-get install numactl

1.3 Benchmarking Procedures
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The run-script runs the GEMM benchmark with a number of threads, data type, and work mode. Then, it will explore the GEMM's matrix size from 256 to 8192.

.. code-block:: bash
 
  ./run_gemm_mkl.sh <thread#> <data_type> <mode>
  
where:

- thread#: Number of threads to run, e.g. 1, 2, 4, 8, 16, etc. (Note: it should not exceed the amout of physical cores.)

- data_type: Either **float** or **double**.

- mode: select **g** for generating the data, **b** for benchmarking the performance, and **a** for both workloads. 

1.4 Result on Nimbix
^^^^^^^^^^^^^^^^^^^^^

tbd

1.5 Reference
^^^^^^^^^^^^^^

[1] `Improving Performance of Math Functions with Intel® Math Kernel Library`_

[2] `Benchmarking GEMM on Intel® Architecture Processors`_

.. _Improving Performance of Math Functions with Intel® Math Kernel Library: https://software.intel.com/en-us/articles/improving-performance-of-math-functions-with-intel-math-kernel-library

.. _Benchmarking GEMM on Intel® Architecture Processors: https://software.intel.com/en-us/articles/benchmarking-gemm-with-intel-mkl-and-blis-on-intel-processors


2. xfblasGemm - Xilinx's GEMX library
--------------------------------------

.. code-block:: c++

  #include "xf_blas.hpp"