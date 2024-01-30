.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.


.. meta::
   :keywords: CRC32
   :description: The hardware resources and performance for CRC32
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


.. _guide_l1_benchmark_crc32:


========
CRC32
========

To profile performance of crc32, prepare a datapack of 268,435,456 byte messages as kernel input.
Based on U50, you have one kernel, each kernel has one PU.
Kernel utilization and throughput are shown in the following table.

Executable Usage
================

* **Work Directory(Step 1)**

The steps for library download and environment setup can be found in :ref:`l1_vitis_security`. To get the design,

.. code-block:: bash

   cd L1/benchmarks/crc32

* **Build kernel(Step 2)**

Run the following make command to build your XCLBIN and host binary targeting a specific device. This process takes long.

.. code-block:: bash

   source /opt/xilinx/Vitis/2022.2/settings64.sh
   source /opt/xilinx/xrt/setenv.sh
   export PLATFORM=u50_gen3x16
   export TARGET=hw
   make run 

* **Run kernel(Step 3)**

To get the benchmark results, run the following command.

.. code-block:: bash

   ./BUILD_DIR/host.exe -xclbin ./BUILD_DIR/CRC32Kernel.xclbin -data PROJECT/data/test.dat -num 16

Input Arguments:

.. code-block:: bash

   Usage: host.exe -[-xclbin]
          -xclbin     binary;

* **Example output(Step 4)**


.. code-block:: bash

   kernel has been created
   kernel start------
   kernel end------
   Execution time 42.357ms
   Write DDR Execution time 1.17767 ms
   Kernel Execution time 40.8367 ms
   Read DDR Execution time 0.047911 ms
   Total Execution time 42.1537 ms


Profiling 
=========

The CRC32 is validated on an AMD Alveo |trade| U50 board. 
Its resource, frequency, and throughput are shown below.

+-----------+----------------+----------------+--------------+-------+----------+-------------+
| Frequency |       LUT      |       REG      |     BRAM     |  URAM |    DSP   |  Throughput |
+-----------+----------------+----------------+--------------+-------+----------+-------------+
| 300 MHz   |     5,322      |     10,547     |     16       |   0   |     0    |   4.7 GB/s  |
+-----------+----------------+----------------+--------------+-------+----------+-------------+


.. toctree::
   :maxdepth: 1

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim: