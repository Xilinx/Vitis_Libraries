.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

==========
Benchmark 
==========
    
.. _datasets:

Datasets
-----------

There are two tables, named ``Lineitem`` and ``Orders``, which are filled with random data.

Performance
-----------

For representing the resource utilization in each benchmark, the overall utilization is separated into two parts, where P stands for the resource usage in platform, that is those instantiated in static region of the FPGA card, as well as K represents those used in kernels (dynamic region). The target device is set to the AMD Alveo™ U280.

.. table:: Table 1 Performance on a FPGA
    :align: center

    +-------------------------------+----------------------------+--------------+----------+-----------------+------------+------------+------------+
    |     Architecture              |     Dataset                |  Latency(ms) |  Timing  |   LUT(P/K)      |  BRAM(P/K) |  URAM(P/K) |  DSP(P/K)  |
    +===============================+============================+==============+==========+=================+============+============+============+
    |  Compound Sort (U280)         |  Orders 131072 rows        |    1.130     |  287 MHz  |  142.0K/62.7K   |   285/18   |    0/16    |    7/0     |
    +-------------------------------+----------------------------+--------------+----------+-----------------+------------+------------+------------+
    |  Hash Anti-Join (U280)        |  Lineitem 6001215 rows     |    342.568   |  250 MHz  |  130.4K/134.6K  |   204/291  |    0/192   |    4/99    |
    |                               |  Orders 227597 rows        |              |          |                 |            |            |            |
    +-------------------------------+----------------------------+--------------+----------+-----------------+------------+------------+------------+
    |  Hash Group Aggregate (U280)  |  Lineitem 6000000 rows     |    34.702    |  200 MHz  |  203.0K/184.1K  |   427/207  |    0/256   |    10/0    |
    +-------------------------------+----------------------------+--------------+----------+-----------------+------------+------------+------------+
    |  Hash Join V2 (U280)          |  Lineitem 6001215 rows     |    55.95     |  282 MHz  |  122.1K/63.7K   |   202/98   |    0/64    |    4/3     |
    |                               |  Orders 227597 rows        |              |          |                 |            |            |            |
    +-------------------------------+----------------------------+--------------+----------+-----------------+------------+------------+------------+
    |  Hash Join V3 (U280)          |  Lineitem 6001215 rows     |    65.26     |  240 MHz  |  197.0K/128.2K  |   359/239  |    0/192   |    10/99   |
    |                               |  Orders 227597 rows        |              |          |                 |            |            |            |
    +-------------------------------+----------------------------+--------------+----------+-----------------+------------+------------+------------+
    |  Hash Join V4 (U280)          |  Lineitem 6001215 rows     |    1354.795  |  240 MHz  |  201.5/110.1K   |   359/187  |    0/256   |    10/19   |
    |                               |  Orders 227597 rows        |              |          |                 |            |            |            |
    +-------------------------------+----------------------------+--------------+----------+-----------------+------------+------------+------------+
    |  Hash Multi-Join (U280)       |  Lineitem 6001215 rows     |    76.899    |  200 MHz  |  130.6K/133.4K  |   204/271  |    0/192   |    4/99    |
    |                               |  Orders 1500000 rows       |              |          |                 |            |            |            |
    +-------------------------------+----------------------------+--------------+----------+-----------------+------------+------------+------------+
    |  Hash Semi-Join (U280)        |  Lineitem 6001215 rows     |    18.914    |  274 MHz  |  124.0K/67.6K   |   202/120  |     0/64   |    4/3     |
    |                               |  Orders 1500000 rows       |              |          |                 |            |            |            |
    +-------------------------------+----------------------------+--------------+----------+-----------------+------------+------------+------------+

These are details for the benchmark result and usage steps:

.. toctree::
   :maxdepth: 1

   Compound Sort <benchmark/compound_sort.rst>
   Hash Anti-join <benchmark/hash_anti_join.rst>
   Hash Group Aggregate <benchmark/hash_group_aggregate.rst>
   Hash Join V2 <benchmark/hash_join_v2.rst>
   Hash Join V3 <benchmark/hash_join_v3.rst>
   Hash Join V4 <benchmark/hash_join_v4.rst>
   Hash Multi-Join <benchmark/hash_multi_join.rst>
   Hash Semi-Join <benchmark/hash_semi_join.rst>

Test Overview
--------------

Here are benchmarks of the AMD Vitis™ Database Library using the Vitis environment:

.. _l2_vitis_database:


* **Download the code**

These database benchmarks can be downloaded from the `vitis libraries <https://github.com/Xilinx/Vitis_Libraries.git>`_ ``main`` branch.

.. code-block:: bash

   git clone https://github.com/Xilinx/Vitis_Libraries.git 
   cd Vitis_Libraries
   git checkout main
   cd database

* **Set up the environment**

Specify the corresponding Vitis, XRT, and path to the platform repository by running the following commands:

.. code-block:: bash

   source <intstall_path>/settings64.sh
   source /opt/xilinx/xrt/setup.sh
   export PLATFORM_REPO_PATHS=/opt/xilinx/platforms