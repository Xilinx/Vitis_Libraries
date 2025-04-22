.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _module_benchmark:

=========================
Benchmark
=========================

1. Performance
=========================

- Kernel execution time only includes the kernel running in FPGA device time.
- API execution time includes the kernel execution time + memory copy between the host and kernel time.


1.1 gemm
---------------

This benchmark performs the matrix-matrix multiplication (A * B = C); M is number of rows of matrix A/C, K is number of columns of matrix A/number of rows of matrix B, and N is number of columns of matrix B/C.

*gemm with OpenCL in an Alveo U250*

+------+------+------+------------------------------+--------------------------+-----------------+
|  M   |  N   |  K   |  Kernel Execution Time [ms]  |  API Execution Time [ms] | Kernel Eff [%]  |  
+======+======+======+==============================+==========================+=================+
| 64   | 64   | 64   | 0.010905                     | 1.750123                 | 38.802577       | 
+------+------+------+------------------------------+--------------------------+-----------------+
| 128  | 128  | 128  | 0.048517                     | 13.802416                | 69.772592       | 
+------+------+------+------------------------------+--------------------------+-----------------+
| 256  | 256  | 256  | 0.328314                     | 14.645931                | 82.485022       | 
+------+------+------+------------------------------+--------------------------+-----------------+
| 512  | 512  | 512  | 3.213388                     | 18.199255                | 67.420400       | 
+------+------+------+------------------------------+--------------------------+-----------------+
| 1024 | 1024 | 1024 | 24.113855                    | 45.519852                | 71.875005       | 
+------+------+------+------------------------------+--------------------------+-----------------+
| 2048 | 2048 | 2048 | 186.688153                   | 264.195138               | 74.270743       | 
+------+------+------+------------------------------+--------------------------+-----------------+
| 4096 | 4096 | 4096 | 1469.773731                  | 1708.938204              | 75.469945       | 
+------+------+------+------------------------------+--------------------------+-----------------+

For more details on this benchmark, see:

.. toctree::
   :maxdepth: 1
   
   L2 GEMM benchmark <user_guide/L2/L2_benchmark_gemm.rst>

*gemm with XRT in an Alveo U250*

+------+------+------+----------------------------+--------------+---------------+
|  M   |  N   |  K   |  api execution time [ms]   | api Eff [%]  |  PerfApiTops  |
+======+======+======+============================+==============+===============+
| 256  | 256  | 256  | 2.295277                   | 11.798572    | 0.058818      |
+------+------+------+----------------------------+--------------+---------------+
| 512  | 512  | 512  | 7.185994                   | 30.148638    | 0.149859      |
+------+------+------+----------------------------+--------------+---------------+
| 1024 | 1024 | 1024 | 33.357721                  | 51.957490    | 0.257887      |
+------+------+------+----------------------------+--------------+---------------+
| 2048 | 2048 | 2048 | 218.662946                 | 63.410230    | 0.314501      |
+------+------+------+----------------------------+--------------+---------------+
| 4096 | 4096 | 4096 | 1594.648667                | 69.559988    | 0.344877      |
+------+------+------+----------------------------+--------------+---------------+
| 8192 | 8192 | 8192 | 12695.637510               | 69.897233    | 0.346485      |
+------+------+------+----------------------------+--------------+---------------+

*gemm with XRT (one CU, streaming Kernel) in an Alveo U250*

+------+------+------+----------------------------+--------------+---------------+
|  M   |  N   |  K   |  api execution time [ms]   | api Eff [%]  |  PerfApiTops  |
+======+======+======+============================+==============+===============+
| 256  | 256  | 256  | 1.370527                   | 19.127241    | 0.024626      |
+------+------+------+----------------------------+--------------+---------------+
| 512  | 512  | 512  | 4.517989                   | 46.417820    | 0.059589      |
+------+------+------+----------------------------+--------------+---------------+
| 1024 | 1024 | 1024 | 29.500145                  | 56.871639    | 0.072902      |
+------+------+------+----------------------------+--------------+---------------+
| 2048 | 2048 | 2048 | 217.555482                 | 61.693563    | 0.079026      |
+------+------+------+----------------------------+--------------+---------------+
| 4096 | 4096 | 4096 | 1685.337895                | 63.710774    | 0.081580      |
+------+------+------+----------------------------+--------------+---------------+

For more details on the benchmarks, see:

.. toctree::
   :maxdepth: 1
   
   L3 API GEMM benchmark <user_guide/L3/L3_benchmark_gemm.rst>


2. Benchmark Test Overview
============================

Here are benchmarks of the AMD Vitis™ BLAS library using the Vitis environment. It supportshardware emulation as well as running hardware accelerators on the Alveo U250.

2.1 Prerequisites
----------------------

2.1.1 Vitis BLAS Library
^^^^^^^^^^^^^^^^^^^^^^^^^^^

- Alveo U250 installed and configured as per https://www.xilinx.com/products/boards-and-kits/alveo/u250.html#gettingStarted (when running hardware)
- XRT installed
- Vitis 2025.1 installed and configured

2.2 Building
----------------

2.2.1 Download Code
^^^^^^^^^^^^^^^^^^^^^

These BLAS benchmarks can be downloaded from the [vitis libraries](https://github.com/Xilinx/Vitis_Libraries.git) ``main`` branch.

.. code-block:: bash 

   git clone https://github.com/Xilinx/Vitis_Libraries.git
   cd Vitis_Libraries
   git checkout main
   cd blas

   
2.2.2 Set Up the Environment
^^^^^^^^^^^^^^^^^^^^^^^^^^

Set up and build the environment using the Vitis and XRT scripts:

.. code-block:: bash 

    source <install path>/Vitis/2025.1/settings64.sh
    source /opt/xilinx/xrt/setup.sh
