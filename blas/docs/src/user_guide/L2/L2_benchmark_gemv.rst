.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: BLAS, Library, Vitis BLAS Library, L2, level 2
   :description: Vitis BLAS library level 2 application programming interface reference. Intel Math Kernel Library provides performance improvement of math functions, e.g. GEMM, when running with Intel processors.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


.. _benchmark_gemv_l2:

***********************
L2 GEMV Benchmark
***********************

1. gemvStreamCh16
=====================

This example resides in the ``L2/benchmarks/streamingKernel/gemvStreamCh16`` directory. The tutorial provides a step-by-step guide that covers commands for building and running the kernel. It performs the matrix-vecotr multiplication; M is number of rows of matrix, and N is number of columns of matrix.

1.1 Executable Usage
------------------------

1.1.1 Work Directory (Step 1)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The steps for library download and environment setup can be found [here](https://github.com/Xilinx/Vitis_Libraries/tree/main/blas/L2/benchmarks#building). For getting the design:

.. code-block:: bash 

    cd L2/benchmarks/streamingKernel/gemvStreamCh16


1.1.2 Build the Kernel (Step 2)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Run the following make command to build your XCLBIN and host binary targeting a specific device. This process will take a long time, maybe couple of hours.

.. code-block:: bash 

    make run TARGET=hw PLATFORM_REPO_PATHS=/opt/xilinx/platforms PLATFORM=xilinx_u280_xdma_201920_1


1.1.3 Run the Kernel (Step 3)
^^^^^^^^^^^^^^^^^^^^^^^^^^^

To get the benchmark results, run the following command:

gemvStreamCh16 Input Arguments:

.. code-block:: bash 

    <host application> <xclbin> <m> <n> <path_to_data> device_id
    

For example:

.. code-block:: bash 

    build_dir.hw.xilinx_u280_xdma_201920_1/host.exe build_dir.hw.xilinx_u280_xdma_201920_1/gemv.xclbin 512 256 build_dir.hw.xilinx_u280_xdma_201920_1/data/ 0


1.1.4 Example Output (Step 4)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash 

    Found Platform
    Platform Name: Xilinx
    INFO: Importing gemv.xclbin
    Loading: 'gemv.xclbin'
    Software-measured execution time 0.000292705s.
    Software-measured HW efficiency 2.09904%.
    Execution clock cycles is: 4759
    Efficiency is: 43.0343%.
    Results verified.


1.2 Profiling for the Alveo U280
-------------------------

The xclbin can be built in 319 MHz.
The hardware resource utilization and benchmark results are shown in the following two tables.

*Table 1 Hardware Resources*

+---------------------+-------------------+------------------+-------------------+----------------+---------------+----------------+
| Name                | LUT               | LUTAsMem         | REG               | BRAM           | URAM          | DSP            |
+=====================+===================+==================+===================+================+===============+================+
| krnl_gemv           |  122248 [ 10.48%] |  11010 [  1.90%] |  215381 [  9.02%] |   72 [  3.97%] |   0 [  0.00%] |  966 [ 10.71%] |
| streamTimer         |     195 [  0.02%] |      0 [  0.00%] |     291 [  0.01%] |    0 [  0.00%] |   0 [  0.00%] |    0 [  0.00%] |
+---------------------+-------------------+------------------+-------------------+----------------+---------------+----------------+

*Table 2 Benchmark Results* 

+-------+-------+---------------------------+-------------------------+-----------------+
|  M    |  N    | Kernel Execution Time [s] | API Execution Time [s]  |  Efficiency [%] |
+=======+=======+===========================+=========================+=================+
| 512   | 256   | 1.4316e-05                | 0.00330468              | 42.9173         |
+-------+-------+---------------------------+-------------------------+-----------------+
| 512   | 512   | 1.9998e-05                | 0.00337302              | 61.4461         |
+-------+-------+---------------------------+-------------------------+-----------------+
| 1024  | 1024  | 6.5904e-05                | 0.0035207               | 74.5812         |
+-------+-------+---------------------------+-------------------------+-----------------+
| 2048  | 2048  | 0.000235251               | 0.00365028              | 83.5737         |
+-------+-------+---------------------------+-------------------------+-----------------+
| 4096  | 4096  | 0.000939699               | 0.00452506              | 83.6898         |
+-------+-------+---------------------------+-------------------------+-----------------+
| 8192  | 8192  | 0.00332612                | 0.0105467               | 94.5764         |
+-------+-------+---------------------------+-------------------------+-----------------+


1.3 Profiling for the Alveo U50
-----------------------

The xclbin can be built in 333 MHz.
The hardware resource utilization and benchmark results are shown in the following two tables.

*Table 1 Hardware Resources*

+---------------------+------------------+------------------+-------------------+----------------+---------------+----------------+
| Name                | LUT              | LUTAsMem         | REG               | BRAM           | URAM          | DSP            |
+=====================+==================+==================+===================+================+===============+================+
| krnl_gemv           | 121535 [ 16.26%] |  11002 [  2.85%] |  215897 [ 13.72%] |   72 [  6.19%] |   0 [  0.00%] |  966 [ 16.27%] |
+---------------------+------------------+------------------+-------------------+----------------+---------------+----------------+
| streamTimer         |    195 [  0.03%] |      0 [  0.00%] |     291 [  0.02%] |    0 [  0.00%] |   0 [  0.00%] |    0 [  0.00%] |
+---------------------+------------------+------------------+-------------------+----------------+---------------+----------------+

*Table 2 Benchmark Results* 

+-------+-------+-----------------------+------------------------------+----------------------------+--------------------------+--------------+
|  M    |  N    | HW Execution Time (s) | Cold API Execution Time (s)  | Hot API Execution Time (s) |  Execution Clock Cycles  |  Efficiency  |
+=======+=======+=======================+==============================+============================+==========================+==============+
| 512   | 256   | 1.4481e-05            | 0.000241345                  | 0.00014245                 | 4827                     | 42.428%      |
+-------+-------+-----------------------+------------------------------+----------------------------+--------------------------+--------------+
| 512   | 512   | 2.0853e-05            | 0.000428344                  | 0.000136975                | 6951                     | 58.9268%     |
+-------+-------+-----------------------+------------------------------+----------------------------+--------------------------+--------------+
| 1024  | 1024  | 6.6462e-05            | 0.000439357                  | 0.00017869                 | 22154                    | 73.955%      |
+-------+-------+-----------------------+------------------------------+----------------------------+--------------------------+--------------+
| 2048  | 2048  | 0.000248076           | 0.000637851                  | 0.000367888                | 82692                    | 79.2531%     |
+-------+-------+-----------------------+------------------------------+----------------------------+--------------------------+--------------+
| 4096  | 4096  | 0.000898929           | 0.00156095                   | 0.00101729                 | 299643                   | 87.4854%     |
+-------+-------+-----------------------+------------------------------+----------------------------+--------------------------+--------------+
| 8192  | 8192  | 0.00332855            | 0.00478017                   | 0.00365307                 | 1109516                  | 94.5075%     |
+-------+-------+-----------------------+------------------------------+----------------------------+--------------------------+--------------+

