.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _l2_manual_shortpath:   

===========================
Single Source Shortest Path
===========================

Single Source Shortest Path(SSSP) example resides in ``L2/benchmarks/shortest_path_float_pred`` directory. The tutorial provides a step-by-step guide that covers commands for building and running kernel.

Executable Usage
================

* **Work Directory(Step 1)**

The steps for library download and environment setup can be found in :ref:`l2_vitis_graph`. To get the design,

.. code-block:: bash

   cd L2/benchmarks/shortest_path_float_pred

* **Build kernel(Step 2)**

Run the following make command to build your XCLBIN and host binary targeting a specific device. This process takes long.

.. code-block:: bash

   make run TARGET=hw PLATFORM=xilinx_u50_gen3x16_xdma_201920_3

* **Run kernel(Step 3)**

To get the benchmark results, run the following command.

.. code-block:: bash

   ./build_dir.hw.xilinx_u50_gen3x16_xdma_201920_3/host.exe -xclbin build_dir.hw.xilinx_u50_gen3x16_xdma_201920_3/shortestPath_top.xclbin -o data/data-csr-offset.mtx -c data/data-csr-indicesweights.mtx -g data/data-golden.sssp.mtx 

Single Source Shortest Path Input Arguments:

.. code-block:: bash

   Usage: host.exe -[-xclbin -o -c -g]
          -xclbin      Xclbin File Name
          -o           Offset File Name
          -c           Indices File Name
          -g           Golden File Name

.. Note:: Default arguments are set in Makefile, you can use other :ref:`datasets` listed in the table.  

* **Example output(Step 4)**

.. code-block:: bash

   ---------------------Shortest Path---------------- 
   id: 92 max out: 13
   Found Platform
   Platform Name: Xilinx
   Found Device=xilinx_u50_gen3x16_xdma_201920_3
   INFO: Importing build_dir.hw.xilinx_u50_gen3x16_xdma_201920_3/shortestPath_top.xclbin
   Loading: 'build_dir.hw.xilinx_u50_gen3x16_xdma_201920_3/shortestPath_top.xclbin'
   kernel has been created
   kernel start------
   kernel call success
   kernel call finish
   kernel end------
   Execution time 11.651ms
   Write DDR Execution time 0.273831ms
   Kernel[0] Execution time 11.2184ms
   Read DDR Execution time 0.130417ms
   Total Execution time 11.6227ms

Profiling
=========

The hardware resource utilizations are listed in the following table.

.. table:: Table 1 Hardware resources
    :align: center

    +---------------+----------+----------+----------+---------+-----------------+
    |  Kernel       |   BRAM   |   URAM   |    DSP   |   LUT   | Frequency(MHz)  |
    +---------------+----------+----------+----------+---------+-----------------+
    |  sssp_Kernel  |    127   |    20    |    2     |  21634  |      300        |
    +---------------+----------+----------+----------+---------+-----------------+

The performance is shown below.

.. table:: Table 2 Performance
    :align: center

    +------------------+----------+----------+-----------+
    |                  |          |          |           |
    | Datasets         | Vertex   | Edges    |  u50 time | 
    |                  |          |          |  (ms)     |
    +------------------+----------+----------+-----------+
    | as-Skitter       | 1694616  | 11094209 |    193.02 |
    +------------------+----------+----------+-----------+
    | coPapersDBLP     | 540486   | 15245729 |    511.55 |
    +------------------+----------+----------+-----------+
    | coPapersCiteseer | 434102   | 16036720 |     38.92 |
    +------------------+----------+----------+-----------+
    | cit-Patents      | 3774768  | 16518948 |    166.03 |
    +------------------+----------+----------+-----------+
    | europe_osm       | 50912018 | 54054660 |    159.94 |
    +------------------+----------+----------+-----------+
    | hollywood        | 1139905  | 57515616 |   7445.04 |
    +------------------+----------+----------+-----------+
    | soc-LiveJournal1 | 4847571  | 68993773 |  24330.10 |
    +------------------+----------+----------+-----------+
    | ljournal-2008    | 5363260  | 79023142 |  24985.60 |
    +------------------+----------+----------+-----------+
