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
Intel® Math Kernel Library provides performance improvement of math functions, e.g. GEMM, when running with Intel processors. To compare with Xilinx's XFBLAS library, you can use our run-script (run_gemm_mkl.sh) to generate the data and performance benchmark.

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

The run-script runs the GEMM benchmark with a number of threads, data type, and work mode. Then, it will explore the GEMM's matrix size from 256 to 16384.

.. code-block:: bash
 
  ./run_gemm_mkl.sh <thread#> <data_type> <mode>
  
.. rubric:: where:

- thread#: Number of threads to run, e.g. 1, 2, 4, 8, 16, etc.

- data_type: Either **float** or **double**.

- mode: **g** for generating the data, **b** for benchmarking the performance, and **a** for both workloads. 

1.4 Running on Nimbix Cloud
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- Follow the user guide `Vitis On Nimbix`_ to login to your Nimbix account
- Launch application "Xilinx SDAccel Development & Alveo FPGA 2018.3" and select "Desktop Mode with FPGA"
- Choose machine type "16 core, 128 GB RAM, Xilinx Alveo U200 FPGA (nx5u_xdma_201830_1)"
- Copy the L3/bencharks/gemm directory to the Nimbix machine, and navigate to the gemm/gemm_mkl directory
- Run Intel® MKL GEMM APIs according to the above benchmark procedures.

.. _Vitis On Nimbix: https://www.xilinx.com/support/documentation/sw_manuals/xilinx2018_3/ug1240-sdaccel-nimbix-getting-started.pdf

.. NOTE:: FPGA is not required in Intel® Math Kernel Library but will be used in Xilinx's XFBLAS library.

1.5 Performance Result on Nimbix Cloud
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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
		
.. rubric:: Performance Result (nonCaching):

+-------------+--------------+------------+-------------+
| Matrix Size | Cache (Y/N)  | TimeApiMS  | PerfApiTops |
+=============+==============+============+=============+
| 256         | N            |   29.700   | 0.001       |
+-------------+--------------+------------+-------------+
| 512         | N            |   11.799   | 0.023       |
+-------------+--------------+------------+-------------+
| 1024        | N            |   16.591   | 0.129       |
+-------------+--------------+------------+-------------+
| 2048        | N            |   41.319   | 0.416       |
+-------------+--------------+------------+-------------+
| 4096        | N            |  172.369   | 0.797       |
+-------------+--------------+------------+-------------+
| 8192        | N            | 1073.250   | 1.024       |
+-------------+--------------+------------+-------------+
| 16384       | N            | 9060.830   | 0.971       |
+-------------+--------------+------------+-------------+

.. rubric:: Performance Result (Caching):

+-------------+--------------+------------+-------------+
| Matrix Size | Cache (Y/N)  | TimeApiMS  | PerfApiTops |
+=============+==============+============+=============+
| 256         | Y            |    1.380   | 0.024       |
+-------------+--------------+------------+-------------+
| 512         | Y            |    4.038   | 0.066       |
+-------------+--------------+------------+-------------+
| 1024        | Y            |    4.383   | 0.490       |
+-------------+--------------+------------+-------------+
| 2048        | Y            |   21.282   | 0.807       |
+-------------+--------------+------------+-------------+
| 4096        | Y            |  149.755   | 0.918       |
+-------------+--------------+------------+-------------+
| 8192        | Y            | 1042.860   | 1.054       |
+-------------+--------------+------------+-------------+
| 16384       | Y            | 9045.700   | 0.972       |
+-------------+--------------+------------+-------------+


1.6 Reference
^^^^^^^^^^^^^^

[1] `Improving Performance of Math Functions with Intel® Math Kernel Library`_

[2] `Benchmarking GEMM on Intel® Architecture Processors`_

.. _Improving Performance of Math Functions with Intel® Math Kernel Library: https://software.intel.com/en-us/articles/improving-performance-of-math-functions-with-intel-math-kernel-library

.. _Benchmarking GEMM on Intel® Architecture Processors: https://software.intel.com/en-us/articles/benchmarking-gemm-with-intel-mkl-and-blis-on-intel-processors


2. xfblasGemm - Xilinx's XFBLAS library
----------------------------------------

You can use the run-script to benchmark our Xilinx's XFBLAS library for the GEMM routine and verify with the golden result generated by Intel® Math Kernel Library.

2.1 Benchmarking Procedures
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The run-script runs the GEMM benchmark with xclbin and cfg files. Then, it will explore the GEMM's matrix size from 256 to 8192.

.. code-block:: bash
 
  ./run_gemm_benchmark.sh path_to_xclbin path_to_config_info
  
.. rubric:: where:

- **path_to_xclbin** refers to the location of xclbin 
- **path_to_config_info** refers to the location of cfg file.
  
2.2 Running on Nimbix Cloud
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- Follow the user guide "Vitis On Nimbix" to login to your Nimbix account
- Launch application "Xilinx SDAccel Development & Alveo FPGA 2018.3" and select "Desktop Mode with FPGA"
- Choose machine type "16 core, 128 GB RAM, Xilinx Alveo U200 FPGA (nx5u_xdma_201830_1)"
- Copy the L3/bencharks/gemm directory to the Nimbix machine, and navigate to the gemm directory
- Run Xilinx's XFBLAS APIs according to the above benchmark procedures.

2.3 Performance Result on Nimbix Cloud
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. rubric:: Configuration:

.. list-table::
	:widths: 20 80
	
	*
		- fpga_model
		- Xilinx Alveo U200 FPGA (nx5u_xdma_201830_1)
	*
		- Frequency
		- 124 Mhz
	*
		- data_type
		- float
		
.. rubric:: Performance Result:

+-------------+--------------+------------+-------------+
| Matrix Size | EffApiPct    | TimeApiMS  | PerfApiTops |
+=============+==============+============+=============+
| 256         | 34.859       |  1.516     |      0.022  |
+-------------+--------------+------------+-------------+
| 512         | 70.170       |  6.026     |      0.045  |
+-------------+--------------+------------+-------------+
| 1024        | 89.511       |  37.788    |      0.057  |
+-------------+--------------+------------+-------------+
| 2048        | 93.373       |  289.805   |      0.059  |
+-------------+--------------+------------+-------------+
| 4096        | 96.731       |  2237.969  |      0.061  |
+-------------+--------------+------------+-------------+
| 8192        | 97.281       |  17802.523 |      0.062  |
+-------------+--------------+------------+-------------+
| 16384       | 98.057       | 141292.933 |      0.062  |
+-------------+--------------+------------+-------------+

2.4 Performance Result on Nimbix Cloud (float32, asynchronous)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. rubric:: Configuration:

.. list-table::
	:widths: 20 80
	
	*
		- fpga_model
		- Xilinx Alveo U200 FPGA (nx5u_xdma_201830_2)
	*
		- kernel#
		- 2
	*
		- Frequency
		- 114 Mhz
	*
		- data_type
		- float32
		
.. rubric:: Performance Result:

+-------------+--------------+------------+-------------+
| Matrix Size | EffApiPct (%)| TimeApiMS  | PerfApiTops |
+=============+==============+============+=============+
| 256         | 28.778       |  1.998     |      0.034  |
+-------------+--------------+------------+-------------+
| 512         | 61.213       |  7.513     |      0.072  |
+-------------+--------------+------------+-------------+
| 1024        | 81.209       |  45.306    |      0.095  |
+-------------+--------------+------------+-------------+
| 2048        | 88.514       |  332.532   |      0.103  |
+-------------+--------------+------------+-------------+
| 4096        | 93.797       |  2510.409  |      0.110  |
+-------------+--------------+------------+-------------+
| 8192        | 95.073       |  19813.749 |      0.111  |
+-------------+--------------+------------+-------------+
| 16384       | 95.801       | 157306.027 |      0.112  |
+-------------+--------------+------------+-------------+

2.5 Performance Result on Nimbix Cloud (int16, asynchronous)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. rubric:: Configuration:

.. list-table::
	:widths: 20 80
	
	*
		- fpga_model
		- Xilinx Alveo U200 FPGA (nx5u_xdma_201830_2)
	*
		- kernel#
		- 2
	*
		- Frequency
		- 240 Mhz
	*
		- data_type
		- int16
		
.. rubric:: Performance Result:

+-------------+--------------+------------+-------------+
| Matrix Size | EffApiPct (%)| TimeApiMS  | PerfApiTops |
+=============+==============+============+=============+
| 256         | 5.383        |  1.268     |      0.053  |
+-------------+--------------+------------+-------------+
| 512         | 21.094       |  2.589     |      0.208  |
+-------------+--------------+------------+-------------+
| 1024        | 39.554       |  11.046    |      0.389  |
+-------------+--------------+------------+-------------+
| 2048        | 62.193       |  56.200    |      0.612  |
+-------------+--------------+------------+-------------+
| 4096        | 73.463       |  380.626   |      0.722  |
+-------------+--------------+------------+-------------+
| 8192        | 76.867       |  2910.186  |      0.756  |
+-------------+--------------+------------+-------------+
| 16384       | 77.626       |  23053.77  |      0.763  |
+-------------+--------------+------------+-------------+

2.6 Performance Result on xbxcloud5 (int16, asynchronous)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. rubric:: Configuration:

.. list-table::
	:widths: 20 80
	
	*
		- fpga_model
		- Xilinx Alveo VCU1525 FPGA
	*
		- kernel#
		- 2
	*
		- Frequency
		- 240 Mhz
	*
		- data_type
		- int16
		
.. rubric:: Performance Result:

+-------------+--------------+------------+-------------+
| Matrix Size | EffApiPct (%)| TimeApiMS  | PerfApiTops |
+=============+==============+============+=============+
| 256         | 6.544        |  1.043     |      0.065  |
+-------------+--------------+------------+-------------+
| 512         | 30.082       |  1.816     |      0.297  |
+-------------+--------------+------------+-------------+
| 1024        | 60.016       |  7.280     |      0.591  |
+-------------+--------------+------------+-------------+
| 2048        | 79.433       |  44.003    |      0.781  |
+-------------+--------------+------------+-------------+
| 4096        | 89.734       |  311.611   |      0.882  |
+-------------+--------------+------------+-------------+
| 8192        | 95.224       |  2349.166  |      0.936  |
+-------------+--------------+------------+-------------+
| 16384       | 97.416       |  18370.401 |      0.958  |
+-------------+--------------+------------+-------------+

