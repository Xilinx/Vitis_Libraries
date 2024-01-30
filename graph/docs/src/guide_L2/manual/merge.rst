.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

=========
Merge 
=========

Merge example resides in ``L2/benchmarks/merge`` directory. The tutorial provides a step-by-step guide that covers commands for building and running kernel.

Executable Usage
================

* **Work Directory(Step 1)**

The steps for library download and environment setup can be found in :ref:`l2_vitis_graph`. To get the design,

.. code-block:: bash

   cd L2/benchmarks/merge

* **Build kernel(Step 2)**

Run the following make command to build your XCLBIN and host binary targeting a specific device. This process might take hours.

.. code-block:: bash

   make run TARGET=hw PLATFORM=xilinx_u50_gen3x16_xdma_5_202210_1

* **Run kernel(Step 3)**

To get the benchmark results, run the following command.

.. code-block:: bash

   ./build_dir.sw_emu.xilinx_u50_gen3x16_xdma_5_202210_1/host.exe -xclbin build_dir.sw_emu.xilinx_u50_gen3x16_xdma_5_202210_1/merge_kernel.xclbin -io merge_data/example/exapmle-wt_offset.mtx -ie merge_data/example/exapmle-wt_edge.mtx -iw merge_data/example/exapmle-wt_weight.mtx -ic merge_data/example/exapmle-wt_c.mtx -oo merge_data/example/exapmle-wt_offset_out.mtx -oe merge_data/example/exapmle-wt_edge_out.mtx -ow merge_data/example/exapmle-wt_weight_out.mtx -go merge_golden/example/exapmle-wt_offset_out.mtx -ge merge_golden/example/exapmle-wt_edge_out.mtx -gw merge_golden/example/exapmle-wt_weight_out.mtx

Merge Input Arguments:

.. code-block:: bash

   Usage: host.exe -[-xclbin -io -ie -iw -ic -oo -oe -ow -go -ge -gw]
         -xclbin:           the kernel name
         -io:               the input for offsets
         -ie:               the input for edges
         -iw                the input for weights
         -ic                the input for node's community id
         -oo                the output of offsets
         -oe                the output of edges
         -ow                the output of weights
         -go                the golden for offsets
         -ge                the golden for edges
         -gw                the golden for weights

.. Note:: Default arguments are set in Makefile, the data has only one column that the node's community id is divided by other clustering algorithm, for example, louvain.

* **Example output(Step 4)** 

.. code-block:: bash

  num = 17, numEdges = 56, num_c_out = 4
  Found Platform
  Platform Name: Xilinx
  Info: Context created
  Info: Command queue created
  Found Device=xilinx_u50_gen3x16_xdma_5_202210_1
  INFO: Importing build_dir.sw_emu.xilinx_u50_gen3x16_xdma_5_202210_1/merge_kernel.xclbin
  Loading: 'build_dir.sw_emu.xilinx_u50_gen3x16_xdma_5_202210_1/merge_kernel.xclbin'
  Kernel Name: merge_kernel, CU Number: 0, Thread creation status: success
  Info: Program created
  Info: Kernel created
  kernel has been created
  INFO: kernel start------
  start ComputeCount
  start GetC, num_v=16
  start UpdateCount, num_v=16
  start SetIndexC, num_v=16
  start GetC, num_v=4
  start GetC, num_v=16
  start GetC, num_v=16
  start DoMergeHls
  start LoadCountC, num_c_out=4
  start LoadJump, num_v=16
  start GetV, num_c_out=4
  start GetEW, num_c_out=4
  start HashAggregateDataflow
  start SetV, num_c_out=4
  start SetEW, num_c_out=4
  SetEW: count = 14
  INFO: kernel end------
  INFO: Execution time 14.454ms
  Info: Time in host-to-device: 1.75013ms
  Info: Time in kernel: 12.2135ms
  Info: Time in device-to-host: 0.169928ms
  Test passed
  device process sw_emu_device done

Profiling
=========

The hardware resource utilizations are listed in the following table.
Different tool versions might result in a slightly different resource.

.. table:: Table 1 : Hardware resources for Merge 
    :align: center

    +-------------------+----------+----------+----------+----------+---------+-----------------+
    |    Kernel         |   BRAM   |   URAM   |    DSP   |    FF    |   LUT   | Frequency(MHz)  |
    +-------------------+----------+----------+----------+----------+---------+-----------------+
    |    merge_kernel   |    73    |   163    |    0     |   57990  |  41622  |       240       |
    +-------------------+----------+----------+----------+----------+---------+-----------------+

.. table::  Table 2 : Merge FPGA acceleration benchmark 
    :align: center
 
    +-------------------+-----------+-----------+---------------+------------+------------+--------------------+-----------+-----------+-----------------+---------------------------+--------------+------------+------------+ 
    |    Graphs (M)     |   NV(M)   |   NE(M)   |  Aver Degree  |  NV Out(M) |  NE Out(M) |  Merge in Louvain  |   CPU(s)  |  FPGA(s)  |  Merge Speed Up | Louvain Speed Up Estimate |  Merge Step1 | Step1 ns/V | Step2 ns/E |
    +-------------------+-----------+-----------+---------------+------------+------------+--------------------+-----------+-----------+-----------------+---------------------------+--------------+------------+------------+
    |    as-Skitter     |   1.62    |   10.58   |     6.54      |    0.11    |    0.88    |         25%        |   1.05    |    0.33   |      3.20       |            1.21           |     0.05     |    9.82    |   12.57    |
    +-------------------+-----------+-----------+---------------+------------+------------+--------------------+-----------+-----------+-----------------+---------------------------+--------------+------------+------------+
    |    cit-Patents    |    3.6    |   15.75   |     4.38      |    0.47    |    9.10    |         62%        |   8.13    |    0.82   |      9.90       |            2.26           |     0.169    |    14.92   |   19.73    | 
    +-------------------+-----------+-----------+---------------+------------+------------+--------------------+-----------+-----------+-----------------+---------------------------+--------------+------------+------------+
    | coPapersCiteseers |    0.41   |   15.29   |     36.94     |    0.02    |    0.19    |         11%        |   0.76    |    0.32   |      2.43       |            1.07           |     0.012    |    9.21    |   9.51     |
    +-------------------+-----------+-----------+---------------+------------+------------+--------------------+-----------+-----------+-----------------+---------------------------+--------------+------------+------------+
    |   coPapersDBLP    |    0.52   |   14.54   |     28.21     |    0.03    |    0.49    |         15%        |   0.87    |    0.32   |      2.76       |            1.11           |     0.015    |    9.25    |   9.94     |
    +-------------------+-----------+-----------+---------------+------------+------------+--------------------+-----------+-----------+-----------------+---------------------------+--------------+------------+------------+
    |    europe_osm     |    48.6   |   51.55   |     1.06      |    22.74   |    73.66   |         54%        |   28.45   |    14.0   |      2.04       |            1.38           |      1.7     |    11.13   |   113.6    |
    +-------------------+-----------+-----------+---------------+------------+------------+--------------------+-----------+-----------+-----------------+---------------------------+--------------+------------+------------+
    |  graph500-20-64b  |     1     |   14.97   |      16       |    0.43    |    5.49    |         45%        |   6.69    |    0.54   |      12.42      |            1.71           |     0.045    |    14.31   |   15.73    |    
    +-------------------+-----------+-----------+---------------+------------+------------+--------------------+-----------+-----------+-----------------+---------------------------+--------------+------------+------------+
    |  graph500-21-64b  |     2     |   30.3    |      16       |    0.91    |    13.45   |         48%        |   15.18   |    1.2    |      12.76      |            1.79           |     0.09     |    14.31   |    17.31   |
    +-------------------+-----------+-----------+---------------+------------+------------+--------------------+-----------+-----------+-----------------+---------------------------+--------------+------------+------------+
    |     hollywood     |    1.09   |   54.46   |     50.46     |    0.08    |    0.56    |         5%         |   3.52    |    1.1    |      3.17       |            1.04           |     0.028    |    8.19    |    9.41    |
    +-------------------+-----------+-----------+---------------+------------+------------+--------------------+-----------+-----------+-----------------+---------------------------+--------------+------------+------------+


.. Note::

   1. Merge running on Intel(R) Xeon(R) Silver 4116 CPU @ 2.10GHz, cache(16896 KB), cores(12).
   2. time unit: s.

.. toctree::
    :maxdepth: 1
