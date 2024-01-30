.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.


.. meta::
   :keywords: hmacSha1
   :description: The hardware resources and performance for hmacSha1
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


.. _guide_l1_benchmark_hmacSha1:


========
hmacSha1
========

To profile performance of hmacSha1, prepare a datapack of 512K messages, each message is 1Kbyte,
key length is 256bits. You have four kernels, each kernel has 16 PUs.
Kernel utilization and throughput are shown in the following table.

Executable Usage
================

* **Work Directory(Step 1)**

The steps for library download and environment setup can be found in :ref:`l1_vitis_security`. For getting the design,

.. code-block:: bash

   cd L1/benchmarks/hmacSha1

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

   ./BUILD_DIR/host.exe -xclbin ./BUILD_DIR/hmacSha1Kernel.xclbin -data PROJECT/data/test.dat -num 16

Input Arguments:

.. code-block:: bash

   Usage: host.exe -[-xclbin]
          -xclbin     binary;

* **Example output(Step 4)**

.. code-block:: bash

   Found Platform
   Platform Name: Xilinx
   Selected Device xilinx_u250_xdma_201830_2
   INFO: Importing build_dir.sw_emu.xilinx_u250_xdma_201830_2/aes256CbcEncryptKernel.xclbin
   Loading: 'build_dir.sw_emu.xilinx_u250_xdma_201830_2/aes256CbcEncryptKernel.xclbin'
   Kernel has been created.
   DDR buffers have been mapped/copy-and-mapped
   12 channels, 2 tasks, 64 messages verified. No error found!
   Kernel has been run for 2 times.
   Total execution time 4725069us


Profiling 
=========

The hmacSha1 is validated on an AMD Alveo |trade| U250 board. 
Its resource, frequency, and throughput are shown below.

+-----------+----------------+------------------+--------------+-------+----------+-------------+
| Frequency |      LUT       |        REG       |     BRAM     |  URAM |   DSP    | Throughput  |
+-----------+----------------+------------------+--------------+-------+----------+-------------+
| 227 MHz   |    959,078     |     1,794,522    |      777     |   0   |   56     | 8.0 GB/s    |
+-----------+----------------+------------------+--------------+-------+----------+-------------+


.. toctree::
   :maxdepth: 1

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim: