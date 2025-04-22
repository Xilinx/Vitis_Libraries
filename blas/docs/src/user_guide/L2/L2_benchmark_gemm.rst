.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: BLAS, Library, Vitis BLAS Library, L2, level 2
   :description: Vitis BLAS library level 2 application programming interface reference. Intel Math Kernel Library provides performance improvement of math functions, e.g. GEMM, when running with Intel processors.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


.. _benchmark_gemm_l2:

***********************
L2 GEMM Benchmark
***********************

1. gemm_1CU
================

This example resides in the ``L2/tests/memKernel/gemm_1CU`` directory. The tutorial provides a step-by-step guide that covers commands for building and running the kernel. It performs the matrix-matrix multiplication (A * B = C); M is number of rows of matrix A/C, K is number of columns of matrix A/number of rows of matrix B, and N is number of columns of matrix B/C.

1.1 Executable Usage
------------------------

1.1.1 Work Directory (Step 1)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash 

   cd L2/tests/memKernel/gemm_1CU
   

1.1.2 Build the Kernel (Step 2)
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Run the following make command to build your XCLBIN and host binary targeting a specific device. This process will take a long time, maybe couple of hours.

.. code-block:: bash 

    make run TARGET=hw PLATFORM_REPO_PATHS=/opt/xilinx/platforms PLATFORM=xilinx_u250_gen3x16_xdma_4_1_202210_1


1.1.3 Run the Kernel (Step 3)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To get the benchmark results, run the following command:

gemm_1CU Input Arguments:

.. code-block:: bash 

    <host application> <xclbin> <data>


For example:

.. code-block:: bash 

    build_dir.hw.xilinx_u250_gen3x16_xdma_4_1_202210_1/host.exe build_dir.hw.xilinx_u250_gen3x16_xdma_4_1_202210_1/blas.xclbin build_dir.hw.xilinx_u250_gen3x16_xdma_4_1_202210_1/data


1.1.4 Example Output (Step 4)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash 

    INFO: loading build_dir.hw.xilinx_u250_gen3x16_xdma_4_1_202210_1/data/app.bin of size 73728
    INFO: loaded 73728 bytes from build_dir.hw.xilinx_u250_gen3x16_xdma_4_1_202210_1/data/app.bin
    Found Platform
    Platform Name: Xilinx
    INFO: device name is: xilinx_u250_gen3x16_xdma_shell_4_1
    INFO: Importing build_dir.hw.xilinx_u250_gen3x16_xdma_4_1_202210_1/blas.xclbin
    Loading: 'build_dir.hw.xilinx_u250_gen3x16_xdma_4_1_202210_1/blas.xclbin'
    INFO: created kernels
    loadXclbin  7144.720789 msec
    create kernels  158.030839 msec
    create buffers  0.120546 msec
    INFO: transferred data to kernel 0
    copy to kernels  146.851691 msec
    INFO: Executed kernel 0
    call kernels  0.098555 msec
    INFO: Transferred data from kernel0
    copyFromFpga  0.457479 msec
    total  7450.297623 msec
    subtotalFpga  305.583425 msec
    
    ###########  Op Gemm  ###########
      C = postScale(A * B + X) 64x64 = 64x64 * 64x64 + 64 x 64
      Comparing ...
      Compared 4096 values:  exact match 1281  within tolerance 2815  mismatch 0
    Gemm C Matches

    test_result:pass


1.2 Profiling
----------------

The xclbin could be built in 300 MHz.
The hardware resource utilization and benchmark results are shown in the following two tables.

*Table 1 Hardware Resources*

+------------+----------+--------+-------+--------+---------+
|    Name    |   LUT    |  BRAM  |  URAM |   DSP  |    FF   |
+============+==========+========+=======+========+=========+
| blasKernel | 198418   | 66     | 24    | 1235   | 383276  |
+------------+----------+--------+-------+--------+---------+

*Table 2 gemm_1CU Benchmark Results*

+------+------+------+------------------------------+--------------------------+
|  M   |  N   |  K   |  Kernel Execution Time [ms]  |  API Execution Time [ms] |  
+======+======+======+==============================+==========================+
| 64   | 64   | 64   | 0.098555                     | 305.583425               |
+------+------+------+------------------------------+--------------------------+
