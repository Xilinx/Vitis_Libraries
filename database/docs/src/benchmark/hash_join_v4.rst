.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _l1_hash_join_v4:

============
Hash Join V4
============

Hash Join V4 resides in the ``L1/benchmarks/hash_join_v4_sc`` directory. This benchmark tests the performance of the `hashJoinV4` primitive with the following query:

.. code-block:: bash

   SELECT
          SUM(l_extendedprice * (1 - l_discount)) as revenue
   FROM
          Orders,
          Lineitem
   WHERE
          l_orderkey = o_orderkey
   ;

Here ``Orders`` is a self-made table filled with random data, which contains a column named ``o_orderkey``; ``Lineitem`` is also a table, which contains three columns named ``l_orderkey``, ``l_extendedprice``, and ``l_discount``.

Dataset
=======

This project uses 32-bit data for numeric fields. To benchmark 64-bit performance, edit `host/table_dt.h`, and make `TPCH_INT` an `int64_t`.

Executable Usage
===============

* **Work Directory (Step 1)**

The steps for library download and environment setup can be found in :ref:`l2_vitis_database`. For getting the design:

.. code-block:: bash

   cd L1/benchmarks/hash_join_v4_sc

* **Build Kernel (Step 2)**

Run the following make command to build your XCLBIN and host binary targeting a specific device. This process will take a long time, maybe couple of hours.

.. code-block:: bash

   make run TARGET=hw PLATFORM=xilinx_u280_xdma_201920_3 

* **Run Kernel (Step 3)**

To get the benchmark results, run the following command:

.. code-block:: bash

   ./build_dir.hw.xilinx_u280_xdma_201920_3/test_join.exe -xclbin build_dir.hw.xilinx_u280_xdma_201920_3/hash_join.xclbin

Hash Join V4 Input Arguments:

.. code-block:: bash

   Usage: test_join.exe -xclbin
          -xclbin:      the kernel name

* **Example Output (Step 4)** 

.. code-block:: bash

   ------------- Hash-Join Test ----------------
   Data integer width is 32.
   Host map buffer has been allocated.
   Lineitem 6001215 rows
   Orders 227597rows
   Lineitem table has been read from disk
   Orders table has been read from disk
   INFO: CPU ref matched 13659906 rows, sum = 6446182945969752
   Found Platform
   Platform Name: Xilinx
   Selected Device xilinx_u280_xdma_201920_3
   INFO: Importing build_dir.hw.xilinx_u280_xdma_201920_3/join_kernel.xclbin
   Loading: 'build_dir.hw.xilinx_u280_xdma_201920_3/join_kernel.xclbin'
   Kernel has been created
   DDR buffers have been mapped/copy-and-mapped
   FPGA result 0: 644618294596.9752
   FPGA execution time of 1 runs: 1364680 usec
   Average execution per run: 1364680 usec
   INFO: kernel 0: execution time 1354795 usec
   ---------------------------------------------

Profiling
=========

The hash join v4 design is validated on an AMD Alveo™ U280 board at a 240 MHz frequency. The hardware resource utilizations are listed in the following table.

.. table:: Table 1 Hardware Resources for Hash Join v4
    :align: center

    +----------------+---------------+-----------+------------+----------+
    |      Name      |       LUT     |    BRAM   |    URAM    |    DSP   |
    +----------------+---------------+-----------+------------+----------+
    |    Platform    |     201457    |    359    |     0      |    10    |
    +----------------+---------------+-----------+------------+----------+
    |   join_kernel  |     110093    |    187    |    256     |    19    |
    +----------------+---------------+-----------+------------+----------+
    |   User Budget  |     1101263   |    1657   |    960     |   9014   |
    +----------------+---------------+-----------+------------+----------+
    |   Percentage   |     10.00%    |    11.29% |    26.67%  |   0.21%  |
    +----------------+---------------+-----------+------------+----------+

The performance is shown below. In above test, table ``Lineitem`` has three columns and 6001215 rows, and ``Orders`` has one column and 227597 rows. This means that the design takes 1354.795 ms to process 69.55 MB, so it achieves 51.33 Mb/s throughput.

.. toctree::
   :maxdepth: 1