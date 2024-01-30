.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Vitis, Security, Library, Vitis Security design, benchmark, result
   :description: Vitis Security Library benchmark results.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


.. result:

*********
Benchmark
*********


+------------------+-----------+----------------+----------------+--------------+-------+----------+-------------+
|  API             | Frequency |     LUT        |     REG        |    BRAM      |  URAM |   DSP    | Throughput  |
+------------------+-----------+----------------+----------------+--------------+-------+----------+-------------+
| aes256CbcDecrypt |  286MHz   |    203,595     |    312,900     |     761      |   0   |    29    |   4.7GBps   |
+------------------+-----------+----------------+----------------+--------------+-------+----------+-------------+
| aes256CbcEncrypt |  224MHz   |   1,059,093    |    1,010,145   |     654      |   0   |    152   |   5.5GBps   |
+------------------+-----------+----------------+----------------+--------------+-------+----------+-------------+
| rc4              |  147MHz   |   1,126,259    |    1,120,505   |     640      |   0   |    216   |   3.0GBps   |
+------------------+-----------+----------------+----------------+--------------+-------+----------+-------------+
| hmacSha1         |  227MHz   |    959,078     |    1,794,522   |     777      |   0   |    56    |   8.0 GBps  |
+------------------+-----------+----------------+----------------+--------------+-------+----------+-------------+
| crc32            |  300MHz   |    5,322       |    10,547      |     16       |   0   |    0     |   4.7 GBps  |
+------------------+-----------+----------------+----------------+--------------+-------+----------+-------------+
| adler32          |  262MHz   |    6,348       |    12,232      |     16       |   0   |    0     |   4.1 GBps  |
+------------------+-----------+----------------+----------------+--------------+-------+----------+-------------+


These are the details for benchmark result and usage steps.

.. toctree::
   :maxdepth: 1

   ../guide_L1/benchmark/aes256CbcDecrypt.rst
   ../guide_L1/benchmark/aes256CbcEncrypt.rst
   ../guide_L1/benchmark/hmacSha1.rst
   ../guide_L1/benchmark/rc4.rst
   ../guide_L1/benchmark/crc32.rst
   ../guide_L1/benchmark/adler32.rst

Test Overview
--------------

Here are benchmarks of the Vitis Security Library using the Vitis environment,


.. _l1_vitis_security: 


* **Download code**

These solver benchmarks can be downloaded from `vitis libraries <https://github.com/Xilinx/Vitis_Libraries.git>`_ ``main`` branch.

.. code-block:: bash

   git clone https://github.com/Xilinx/Vitis_Libraries.git
   cd Vitis_Libraries
   git checkout main
   cd security

* **Setup environment**

Specify the corresponding Vitis, XRT, and path to the platform repository by running the following commands.

.. code-block:: bash

   source /opt/xilinx/Vitis/2022.2/settings64.sh
   source /opt/xilinx/xrt/setup.sh
   export PLATFORM_REPO_PATHS=/opt/xilinx/platforms
