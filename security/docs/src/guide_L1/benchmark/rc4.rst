.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.


.. meta::
   :keywords: rc4
   :description: The hardware resources and performance for rc4
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


.. _guide_l1_benchmark_rc4:


========
rc4
========

To profile performance of rc4, prepare a datapack of 24 messages, each message is 2Mbyte.
You have four kernels, each kernel has 12 PUs.
Kernel utilization and throughput is shown in the following table.

Executable Usage
================

* **Work Directory(Step 1)**

The steps for library download and environment setup can be found in :ref:`l1_vitis_security`. To get the design,

.. code-block:: bash

   cd L1/benchmarks/rc4

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

   ./BUILD_DIR/host.exe -xclbin ./BUILD_DIR/rc4Kernel.xclbin -data PROJECT/data/test.dat -num 16

Input Arguments:

.. code-block:: bash

   Usage: host.exe -[-xclbin]
          -xclbin     binary;

* **Example output(Step 4)**

.. code-block:: bash

   Info: Program created
   Info: Kernel created
   DDR buffers have been mapped/copy-and-mapped
   Kernel has been run for 2 times.
   Execution time 14388404us
   12 channels, 2 tasks, 2048 messages verified. No error found!
   Info: Test passed

Profiling 
=========

The rc4 is validated on an AMD Alveo |trade| U250 board. 
Its resource, frequency, and throughput are shown below.

+-----------+----------------+----------------+--------------+-------+----------+-------------+
| Frequency |       LUT      |      REG       |     BRAM     |  URAM |   DSP    | Throughput  |
+-----------+----------------+----------------+--------------+-------+----------+-------------+
| 147MHz    |    1,126,259   |   1,120,505    |     640      |  0    |   216    |   3.0GB/s   |
+-----------+----------------+----------------+--------------+-------+----------+-------------+


.. toctree::
   :maxdepth: 1

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
