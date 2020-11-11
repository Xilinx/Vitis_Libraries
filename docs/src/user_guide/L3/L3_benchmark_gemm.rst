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
   :keywords: BLAS, Library, Vitis BLAS Library, L3, level 3
   :description: Vitis BLAS library level 3 application programming interface reference. Intel Math Kernel Library provides performance improvement of math functions, e.g. GEMM, when running with Intel processors.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


.. _benchmark_gemm_l3:

***********************
L3 API GEMM benchmark
***********************

1. Benchmarking Intel® Math Kernel Library (MKL)
=================================================

1.1 Introduction
-----------------

Intel® Math Kernel Library provides performance improvement of math functions, e.g. GEMM, when running with Intel processors. To compare with Xilinx's Vitis BLAS library, you can use our run-script (run_gemm_mkl.sh) to generate the data and performance benchmark.

.. _MKL_benchmark:

1.2 Benchmarking Steps
-----------------------

1.2.1 Access Nimbix cloud
^^^^^^^^^^^^^^^^^^^^^^^^^^

- Follow the user guide `Vitis On Nimbix`_ to login to your Nimbix account
- Launch application "Xilinx Vitis Unified Software Platform 2020.1" and select "Desktop Mode with FPGA"
- Choose machine type "16 core, 128 GB RAM, Xilinx Alveo U250 FPGA (nx6u_xdma_201830_2_2_3)"
- Copy the L3/bencharks/gemm directory to the Nimbix machine, and navigate to the gemm/gemm_mkl directory
- Follow the steps below to run Intel® MKL GEMM APIsbenchmarks.

.. _Vitis On Nimbix: https://www.xilinx.com/xilinxtraining/assessments/portal/alveo/intro_nimbix_cloud/story_html5.html 

.. NOTE:: FPGA is not required in Intel® Math Kernel Library but will be used in Xilinx's Vitis BLAS library.

1.2.2 Install Intel® MK library
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To install MKL on Nimbix, please download the full installation package for MKL2020 from `Intel® MKL Webste`_. You need to register for downloading the package. After you have downloaded the package, please unzip it and navigate to the directory includeing "install.sh". Please enter the following command to install the MKL package.

.. code-block:: bash 
  
  sudo ./install.sh

.. _Intel® MKL Webste: https://software.intel.com/en-us/mkl/choose-download/linux

1.2.3 Set up MKL environment variables
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Intel® MKL**: Assume you have installed Intel® MKL, run the appropriate script to set up the environment variables (such as $MKLROOT).

.. code-block:: bash
 
  source <INTEL_MKL_INSTALL_DIR>/bin/mklvars.sh intel64

1.2.4 Install numactl
^^^^^^^^^^^^^^^^^^^^^^^

**NUMACTL**: The linux operating system provides a function, called numactl, that allows the control of scheduling or memory placement policy, which is essential to run parallel programs.

For Ubuntu (you only need to do it once),

.. code-block:: bash
 
  sudo apt-get install numactl

1.2.5 Run MKL benchmarking script 
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The run-script runs the GEMM benchmark with a number of threads, data type, and work mode. Then, it will explore the GEMM's matrix size from 256 to 16384.

.. code-block:: bash
 
  ./run_gemm_mkl.sh <thread#> <data_type> <mode>
  
.. rubric:: where:

- thread#: Number of threads to run, e.g. 1, 2, 4, 8, 16, etc.

- data_type: Either **float** or **double**.

- mode: **g** for generating the data, **b** for benchmarking the performance, and **a** for both workloads. 

1.3 Performance Result on Nimbix Cloud
---------------------------------------

.. rubric:: Configuration:

.. list-table::
	:widths: 20 80
	
	*
		- cpu_model
		- Intel(R) Xeon(R) CPU E5-2640 v3 @ 2.60GHz
	*
		- thread#
		- 16
	*
		- data_type
		- float
	*
		- benchmark command 
		- ./run_gemm_mkl.sh 16 float a

.. rubric:: Performance Result (nonCaching):

+--------------------+-------------------------------------+-------------+---------------+-------------+
| Square Matrix Size | matrix paris running simultaneously | Cache (Y/N) | API time(ms)  | TFlops/sec  |
+====================+=====================================+=============+===============+=============+
| 256                | 1                                   | N           |   29.700      | 0.001       |
+--------------------+-------------------------------------+-------------+---------------+-------------+
| 512                | 1                                   | N           |   11.799      | 0.023       |
+--------------------+-------------------------------------+-------------+---------------+-------------+
| 1024               | 1                                   | N           |   16.591      | 0.129       |
+--------------------+-------------------------------------+-------------+---------------+-------------+
| 2048               | 1                                   | N           |   41.319      | 0.416       |
+--------------------+-------------------------------------+-------------+---------------+-------------+
| 4096               | 1                                   | N           |  172.369      | 0.797       |
+--------------------+-------------------------------------+-------------+---------------+-------------+
| 8192               | 1                                   | N           | 1073.250      | 1.024       |
+--------------------+-------------------------------------+-------------+---------------+-------------+
| 16384              | 1                                   | N           | 9060.830      | 0.971       |
+--------------------+-------------------------------------+-------------+---------------+-------------+

.. rubric:: Performance Result (Caching):

+--------------------+-------------------------------------+-------------+---------------+-------------+
| Square Matrix Size | matrix paris running simultaneously | Cache (Y/N) | API time(ms)  | TFlops/sec  |
+====================+=====================================+=============+===============+=============+
| 256                | 1                                   | Y           |    1.380      | 0.024       |
+--------------------+-------------------------------------+-------------+---------------+-------------+
| 512                | 1                                   | Y           |    4.038      | 0.066       |
+--------------------+-------------------------------------+-------------+---------------+-------------+
| 1024               | 1                                   | Y           |    4.383      | 0.490       |
+--------------------+-------------------------------------+-------------+---------------+-------------+
| 2048               | 1                                   | Y           |   21.282      | 0.807       |
+--------------------+-------------------------------------+-------------+---------------+-------------+
| 4096               | 1                                   | Y           |  149.755      | 0.918       |
+--------------------+-------------------------------------+-------------+---------------+-------------+
| 8192               | 1                                   | Y           | 1042.860      | 1.054       |
+--------------------+-------------------------------------+-------------+---------------+-------------+
| 16384              | 1                                   | Y           | 9045.700      | 0.972       |
+--------------------+-------------------------------------+-------------+---------------+-------------+


1.4 Reference
--------------

[1] `Improving Performance of Math Functions with Intel® Math Kernel Library`_

[2] `Benchmarking GEMM on Intel® Architecture Processors`_

.. _Improving Performance of Math Functions with Intel® Math Kernel Library: https://software.intel.com/en-us/articles/improving-performance-of-math-functions-with-intel-math-kernel-library

.. _Benchmarking GEMM on Intel® Architecture Processors: https://software.intel.com/en-us/articles/benchmarking-gemm-with-intel-mkl-and-blis-on-intel-processors


2. Benchmarking xfblasGemm - Xilinx's Vitis BLAS library
==========================================================

Before benchmarking xfblashGemm, please run the following command to build hw xclbin

.. code-block:: bash

  make build TARGET=hw PLATFORM_REPO_PATHS=LOCAL_PLATFORM_PATH

2.1 Benchmarking Steps 
------------------------

2.1.1 Generate test inputs and golden reference
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Follow the MKL_benchmark_ steps to run MKL benchmarks to generate test inputs and golden reference. To generate test inputs and golden reference for float data type, please run the following command.

.. code-block:: bash

  ./run_gemm_mkl.sh 16 float a


2.1.2 Build benchmark application
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Before benchmark the xfblasGemm, please build the host executable for the corresponding .xclbin files via following script

.. code-block:: bash

  ./build_gemm_bench.sh config_info_file

2.1.3 Run benchmark
^^^^^^^^^^^^^^^^^^^^

The run-script runs the GEMM benchmark with xclbin and cfg files. It will explore the GEMM's matrix size from 256 to 8192.

.. code-block:: bash
 
  ./run_gemm_benchmark.sh xclbin_file config_info_file
  
.. rubric:: where:

- **xclbin_file** refers to the blas.xclbin file, including the path.
- **config_info_file** refers to config_info.dat file, including the path.
  
2.2 Performance Results on Nimbix Cloud
------------------------------------------

.. rubric:: Configuration:

.. list-table::
	:widths: 20 80

	*
		- fpga_model
		- Xilinx Alveo U250 FPGA
	*
		- Frequency
		- 242 Mhz
	*
		- data_type
		- float
	*
		- build command 
		- ./build_gemm_bench.sh config_info.dat
	*
		- benchmark command
		- ./run_gemm_bench.sh blas.xclbin config_info.dat
		
.. rubric:: Performance Result:

+--------------------+-------------------------------------+--------------+-------------+
| Square Matrix Size | matrix paris running simultaneously | API time(ms) | TFlops/sec  |
+====================+=====================================+==============+=============+
| 256                | 4                                   |  2.348       |      0.057  |
+--------------------+-------------------------------------+--------------+-------------+
| 512                | 4                                   |  6.422       |      0.167  |
+--------------------+-------------------------------------+--------------+-------------+
| 1024               | 4                                   |  33.366      |      0.257  |
+--------------------+-------------------------------------+--------------+-------------+
| 2048               | 4                                   |  217.949     |      0.316  |
+--------------------+-------------------------------------+--------------+-------------+
| 4096               | 4                                   |  1595.487    |      0.344  |
+--------------------+-------------------------------------+--------------+-------------+
| 8192               | 4                                   |  12587.950   |      0.349  |
+--------------------+-------------------------------------+--------------+-------------+

Please notice that we used OpenMP library for multi-kernel support in host side, so for smaller sizes, total API times include OpenMP initialization time.