.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

=========
Renumber 
=========

Renumber example resides in ``L2/benchmarks/renumber`` directory. The tutorial provides a step-by-step guide that covers commands for building and running kernel.

Executable Usage
================

* **Work Directory(Step 1)**

The steps for library download and environment setup can be found in :ref:`l2_vitis_graph`. To get the design,

.. code-block:: bash

   cd L2/benchmarks/renumber

* **Build kernel(Step 2)**

Run the following make command to build your XCLBIN and host binary targeting a specific device. This process takes long.

.. code-block:: bash

   make run TARGET=hw PLATFORM=xilinx_u50_gen3x16_xdma_201920_3

* **Run kernel(Step 3)**

To get the benchmark results, run the following command.

.. code-block:: bash

   ./build_dir.hw.xilinx_u50_gen3x16_xdma_201920_3/host.exe -xclbin build_dir.hw.xilinx_u50_gen3x16_xdma_201920_3/kernel_renumber.xclbin -i data/example.txt

Renumber Input Arguments:

.. code-block:: bash

   Usage: host.exe -[-xclbin -i]
         -xclbin:           the kernel name
         -i:                the input data

.. Note:: Default arguments are set in Makefile, the data has only one column that the node's community id is divided by other clustering algorithm, for example louvain.

* **Example output(Step 4)** 

.. code-block:: bash

  -----------------Renumber----------------
  INFO: numVertices=16 
  Within renumberClustersContiguously()
  INFO: renumberClustersContiguously time 0.0150 ms.
  Found Platform
  Platform Name: Xilinx
  Info: Context created
  Info: Command queue created
  Found Device=xilinx_u50_gen3x16_xdma_201920_3
  INFO: Importing kernel_renumber.xclbin
  Loading: 'kernel_renumber.xclbin'
  Info: Program created
  Info: Kernel created
  kernel has been created
  XRT build version: 2.8.0
  Build hash: e286e561dffa8fe46b74cb36b00b7cac8f8fad68
  Build date: 2021-02-02 21:38:45
  Git branch: HEAD
  PID: 129348
  UID: 35700
  [Wed Aug  4 08:52:03 2021 GMT]
  HOST: xsjkumar50
  EXE: /wrk/xsjhdnobkup5/yuxiangz/project/renumber/single_renumber/data64M/host.exe
  [XRT] WARNING: unaligned host pointer '0x7fff7892a410' detected, this leads to extra memcpy
  INFO: kernel start------
  INFO: kernel end------
  INFO: Execution time 0.59ms
  Info: Time in host-to-device: 0.114176ms
  Info: Time in kernel: 0.279552ms
  Info: Time in device-to-host: 0.0768ms
  INFO: Number of unique clusters 4
  Info: Test passed


Profiling
=========

The hardware resource utilizations are listed in the following table.
Different tool versions might result in slightly different resource.

.. table:: Table 1 : Hardware resources for Renumber 
    :align: center

    +-------------------+----------+----------+----------+----------+---------+-----------------+
    |    Kernel         |   BRAM   |   URAM   |    DSP   |    FF    |   LUT   | Frequency(MHz)  |
    +-------------------+----------+----------+----------+----------+---------+-----------------+
    |  kernel_renumber  |    27    |   256    |    0     |   21692  |  22105  |     240.3       |
    +-------------------+----------+----------+----------+----------+---------+-----------------+

.. table::  Table 2 : Renumber FPGA acceleration benchmark 
    :align: center

    +---------------+-----------+-----------+------------+--------------+
    |   Vertex (M)  |   0.108   |   0.515   |   1.618    |    48.553    |
    +---------------+-----------+-----------+------------+--------------+
    |    CPU (ms)   |   7.19    |   33.91   |   153.31   |   14827.79   |
    +---------------+-----------+-----------+------------+--------------+
    |   FPGA (ms)   |   12.10   |   55.75   |   176.23   |   5215.65    | 
    +---------------+-----------+-----------+------------+--------------+
    |    Speed      |    59%    |    61%    |    87%     |     284%     |
    +---------------+-----------+-----------+------------+--------------+

.. Note::

   1. Renumber running on Intel(R) Xeon(R) Silver 4116 CPU @ 2.10GHz, cache(16896 KB), cores(12).
   2. time unit: ms.

.. toctree::
    :maxdepth: 1
