.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _l1_componentsort:

=============
Compound Sort
=============

The compound sort example resides in the ``L1/benchmarks/compound_sort`` directory.

This benchmark tests the performance of the `compoundSort` primitive with an array of integer keys. This primitive is named as compound sort, as it combines `insertSort` and `mergeSort`, to balance storage and compute resource usage. 

The tutorial provides a step-by-step guide that covers commands for building and running the kernel.

Executable Usage
================

* **Work Directory (Step 1)**

   The steps for library download and environment setup can be found in :ref:`l2_vitis_database`. For getting the design:

.. code-block:: bash

   cd L1/benchmarks/compound_sort

* **Build Kernel (Step 2)**

   Run the following make command to build your XCLBIN and host binary targeting a specific device. This process will take a long time, maybe couple of hours.

.. code-block:: bash

   make run TARGET=hw PLATFORM=xilinx_u280_xdma_201920_3

* **Run Kernel (Step 3)**

To get the benchmark results, run the following command.

.. code-block:: bash

   ./build_dir.hw.xilinx_u280_xdma_201920_3/host.exe -xclbin build_dir.hw.xilinx_u280_xdma_201920_3/SortKernel.xclbin 

Compound Sort Input Arguments:

.. code-block:: bash

   Usage: host.exe -xclbin
          -xclbin     compound sort binary

.. note:: Default arguments are set in the Makefile; you can use other platforms to build and run.

* **Example Output (Step 4)** 

.. code-block:: bash
   
   -----------Sort Design---------------
   key length is 131072
   [INFO]Running in hw mode
   Found Platform
   Platform Name: Xilinx
   Found Device=xilinx_u280_xdma_201920_3
   INFO: Importing build_dir.hw.xilinx_u280_xdma_201920_3/SortKernel.xclbin
   Loading: 'build_dir.hw.xilinx_u280_xdma_201920_3/SortKernel.xclbin'
   kernel has been created
   kernel start------
   PASS!
   Write DDR Execution time 127.131us
   Kernel Execution time 1129.78us
   Read DDR Execution time 83.459us
   Total Execution time 1340.37us
   ------------------------------------------------------------

Profiling 
=========

The compound sort design is validated on an AMD Alveo™ U280 board at a 287 MHz frequency. The hardware resource utilizations are listed in the following table.

.. table:: Table 1 Hardware Resources for Compound Sort
    :align: center

    +------------+--------------+-----------+----------+--------+
    |    Name    |      LUT     |    BRAM   |   URAM   |   DSP  |
    +------------+--------------+-----------+----------+--------+
    | Platform   |    142039    |    285    |    0     |    7   |
    +------------+--------------+-----------+----------+--------+
    | SortKernel |    62685     |    18     |    16    |    0   |
    +------------+--------------+-----------+----------+--------+
    | User Budget|   1160681    |   1731    |    960   |   9017 |
    +------------+--------------+-----------+----------+--------+
    | Percentage |    5.40%     |   1.04%   |   1.67%  |    0   |
    +------------+--------------+-----------+----------+--------+


The performance is as follows. This design takes 1.130 ms to process 0.5 MB data, so it achieves 442.56 Mb/s throughput.

.. toctree::
    :maxdepth: 1