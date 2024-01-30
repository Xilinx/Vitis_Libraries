.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _l1_hash_group_aggregate:

====================
Hash Group Aggregate
====================

Hash Group Aggregate resides in the ``L1/benchmarks/hash_group_aggregate`` directory.

.. code-block:: bash

   SELECT
           max(l_extendedprice), min(l_extendedprice), count_non_zero(l_extendedprice) as revenue
   FROM
           Lineitem
   GROUP BY
           l_orderkey
   ;

Here, ``Lineitem`` is a table filled with random data, which contains two columns named ``l_orderkey`` and ``l_extendedprice``.

Dataset
=======

This project uses 32-bit data for numeric fields. To benchmark 64-bit performance, edit `host/table_dt.h`, and make `TPCH_INT` an `int64_t`.

Executable Usage
===============

* **Work Directory (Step 1)**

The steps for library download and environment setup can be found in :ref:`l2_vitis_database`. For getting the design:

.. code-block:: bash

   cd L1/benchmarks/hash_group_aggregate

* **Build Kernel (Step 2)**

Run the following make command to build your XCLBIN and host binary targeting a specific device. This process will take a long time, maybe couple of hours.

.. code-block:: bash

   make run TARGET=hw PLATFORM=xilinx_u280_xdma_201920_3 

* **Run Kernel (Step 3)**

To get the benchmark results, run the following command:

.. code-block:: bash

   ./build_dir.hw.xilinx_u280_xdma_201920_3/test_aggr.exe -xclbin build_dir.hw.xilinx_u280_xdma_201920_3/hash_aggr_kernel.xclbin

Hash Group Aggregate Input Arguments:

.. code-block:: bash

   Usage: test_aggr.exe -xclbin
          -xclbin:      the kernel name

.. note:: The default arguments are set in the Makefile; you can use other platforms to build and run.

* **Example Output (Step 4)** 

.. code-block:: bash

   ---------- Query with TPC-H 1G Data ----------
   
    select max(l_extendedprice), min(l_extendedprice),
           sum(l_extendedprice), count(l_extendedprice)
    from lineitem
    group by l_orderkey
    ---------------------------------------------
   Host map Buffer has been allocated.
   Lineitem 6000000 rows
   Lineitem table has been read from disk
   insert: idx=0 key=180 i_pld=30bca4
   insert: idx=1 key=377 i_pld=6137c
   ...
   Checking: idx=3e5 key:192 pld:d20c5351
   Checking: idx=3e6 key:385 pld:c074aacf
   Checking: idx=3e7 key:104 pld:e6713746
   No error found!
   kernel done!
   kernel_result_num=0x3e8
   FPGA execution time of 3 runs: 104107 usec
   Average execution per run: 34702 usec
   INFO: kernel 0: execution time 31273 usec
   INFO: kernel 1: execution time 56554 usec
   INFO: kernel 2: execution time 43182 usec
   read_config: pu_end_status_a[0]=0x22222222
   read_config: pu_end_status_b[0]=0x22222222
   read_config: pu_end_status_a[1]=0x08
   read_config: pu_end_status_b[1]=0x08
   read_config: pu_end_status_a[2]=0x08
   read_config: pu_end_status_b[2]=0x08
   read_config: pu_end_status_a[3]=0x3e8
   read_config: pu_end_status_b[3]=0x3e8
   ref_result_num=3e8
   ---------------------------------------------
   PASS!
   
   ---------------------------------------------   

Profiling
=========

The hash group aggregate design is validated on an AMD Alveo™ U280 board at a 200 MHz frequency. The hardware resource utilizations are listed in the following table.

.. table:: Table 1 Hardware Resources for Hash Group Aggregate
    :align: center

    +------------------+---------------+-----------+------------+----------+
    |      Name        |       LUT     |    BRAM   |    URAM    |    DSP   |
    +------------------+---------------+-----------+------------+----------+
    |    Platform      |     202971    |    427    |     0      |    10    |
    +------------------+---------------+-----------+------------+----------+
    | hash_aggr_kernel |     184064    |    207    |    256     |    0     |
    +------------------+---------------+-----------+------------+----------+
    |   User Budget    |     1099749   |    1589   |    960     |   9014   |
    +------------------+---------------+-----------+------------+----------+
    |   Percentage     |     16.74%    |   13.03%  |   26.67%   |    0     |
    +------------------+---------------+-----------+------------+----------+

The performance is shown below. In above test, table ``Lineitem`` has two columns and 6000000 rows. This means that the design takes 34.702 ms to process 45.78 MB data, so it achieves 1.29 Gb/s throughput.

.. toctree::
   :maxdepth: 1