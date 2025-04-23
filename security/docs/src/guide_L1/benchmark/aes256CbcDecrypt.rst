.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.


.. meta::
   :keywords: aes256CbcDecrypt
   :description: The hardware resources and performance for aes256CbcDecrypt
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


.. _guide_l1_benchmark_aes256CbcDecrypt:


=================
aes256CbcDecrypt
=================

To profile performance of aes256CbcDecrypt, prepare a datapack of 32K messages, each message is 1Kbyte.
You have one kernel, each kernel has four PUs.
Kernel utilization and throughput are shown in the following table.

Executable Usage
================

* **Work Directory(Step 1)**

The steps for library download and environment setup can be found in :ref:`l1_vitis_security`. To get the design,

.. code-block:: bash

   cd L1/benchmarks/aes256CbcDecrypt

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

   ./BUILD_DIR/host.exe -xclbin ./BUILD_DIR/aes256CbcDecryptKernel.xclbin

Input Arguments:

.. code-block:: bash

   Usage: host.exe -[-xclbin]
          -xclbin     binary;

* **Example output(Step 4)**

.. code-block::  bash

   Info: Program created
   Info: Kernel created
   Transfer package of 0.0166168 MB to device took 20287us, bandwidth = 0.819086MB/s
   Packages contains additional info, pure message size = 0.015625MB
   Kernel process message of 0.015625 MB took 2.00028e+06us, performance = 0.00781139MB/s
   Transfer package of 0.0158844 MB to host took 20519.4us, bandwidth = 0.774117MB/s
   res num: 16
   Info: Test passed


Profiling 
=========

The aes256CbcDecrypt is validated on AMD Alveo |trade| U250 board. 
Its resource, frequency, and throughput are shown below.  

+-----------+------------+------------+---------+----------+-------+--------------+
|Frequency  |     LUT    |     REG    |   BRAM  |   URAM   |  DSP  |  Throughput  |
+-----------+------------+------------+---------+----------+-------+--------------+
| 286MHz    | 203,595    |  312,900   |  761    |    0     |  29   | 4.7GB/s      |
+-----------+------------+------------+---------+----------+-------+--------------+


.. toctree::
   :maxdepth: 1

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
