.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _l1_hash_multi_join:

===============
Hash Multi-Join
===============

Hash Multi-Join resides in the ``L1/benchmarks/hash_multi_join`` directory. This benchmark tests the performance of the `hashMultiJoin` primitive with the following query:

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

   cd L1/benchmarks/hash_multi_join

* **Build Kernel (Step 2)**

Run the following make command to build your XCLBIN and host binary targeting a specific device. This process will take a long time, maybe couple of hours.

.. code-block:: bash

   make run TARGET=hw PLATFORM=xilinx_u280_xdma_201920_3 

* **Run Kernel (Step 3)**

To get the benchmark results, run the following command:

.. code-block:: bash

   ./build_dir.hw.xilinx_u280_xdma_201920_3/test_join.exe -xclbin build_dir.hw.xilinx_u280_xdma_201920_3/hash_multi_join.xclbin

Hash Multi-Join Input Arguments:

.. code-block:: bash

   Usage: test_join.exe -xclbin
          -xclbin:      the kernel name

.. note:: The default arguments are set in the Makefile; you can use other platforms to build and run.

* **Example Output (Step 4)** 

.. code-block:: bash

   ------------- Hash-Join Test ----------------
   Data integer width is 32.
   Host map buffer has been allocated.
   Lineitem 6001215 rows
   Orders 227597rows
   Lineitem table has been read from disk
   Orders table has been read from disk
   INFO: CPU ref matched 5389889 rows, sum = 2543814013792128
   Found Platform
   Platform Name: Xilinx
   Selected Device xilinx_u280_xdma_201920_3
   INFO: Importing build_dir.hw.xilinx_u280_xdma_201920_3/hash_multi_join.xclbin
   Loading: 'build_dir.hw.xilinx_u280_xdma_201920_3/hash_multi_join.xclbin'
   Kernel has been created
   DDR buffers have been mapped/copy-and-mapped
   FPGA result 0: 254381401379.2128
   Golden result: 254381401379.2128
   FPGA execution time of 1 runs: 84605 usec
   Average execution per run: 84605 usec
   INFO: kernel 0: execution time 76899 usec
   Test Pass
   ---------------------------------------------   

Profiling
=========

The hash multi-join design is validated on an AMD Alveo™ U280 board at a 200 MHz frequency. The hardware resource utilizations are listed in the following table.

.. table:: Table 1 Hardware Resources for Hash Multi-Join
    :align: center

    +----------------+---------------+-----------+------------+----------+
    |      Name      |       LUT     |    BRAM   |    URAM    |    DSP   |
    +----------------+---------------+-----------+------------+----------+
    |    Platform    |     130582    |    204    |     0      |    4     |
    +----------------+---------------+-----------+------------+----------+
    |   join_kernel  |     133390    |    271    |    192     |    99    |
    +----------------+---------------+-----------+------------+----------+
    |   User Budget  |     1172138   |    1812   |    960     |   9020   |
    +----------------+---------------+-----------+------------+----------+
    |   Percentage   |     11.38%    |    14.96% |  20.00%    |   1.10%  |
    +----------------+---------------+-----------+------------+----------+

The performance is shown below. In above test, table ``Lineitem`` has three columns and 6001215 rows, and ``Orders`` has one column and 227597 rows. This means that the design takes 76.899 ms to process 69.55 MB data, so it achieves 0.88 Gb/s throughput.

.. toctree::
   :maxdepth: 1