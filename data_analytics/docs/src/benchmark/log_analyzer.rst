.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _l2_log_analyzer:

============
Log Analyzer
============

Log Analyzer resides in the ``L2/demos/text/log_analyzer`` directory. It is an integration frame included three parts: Grok, GeoIP, and JsonWriter. 


Dataset
=======

- Input log: http://www.almhuette-raith.at/apache-log/access.log (1.2 GB)
- logAnalyzer Demo execute time: 0.99 s, throughput: 1.2 Gb/s
- Baseline `ref_result/ref_result.cpp` execute time: 53.1 s, throughput: 22.6 Mb/s
- Accelaration Ratio: 53X

.. note::
    | 1. The baseline version run on Intel® Xeon® CPU E5-2690 v4, clocked at 2.60 GHz.
    | 2. The baseline version is a single thread program.


Executable Usage
===============

* **Work Directory (Step 1)**

The steps for library download and environment setup can be found in :ref:`l2_vitis_data_analytics`. For getting the design:

.. code-block:: bash

   cd L2/demos/text/log_analyzer

* **Build the Kernel (Step 2)**

Run the following make command to build your XCLBIN and host binary targeting a specific device. This process will take a long time, maybe couple of hours.

.. code-block:: bash

   make run TARGET=hw PLATFORM=xilinx_u200_xdma_201830_2 

* **Run the Kernel (Step 3)**

To get the benchmark results, run the following command:

.. code-block:: bash

   ./build_dir.hw.xilinx_u200_xdma_201830_2/test.exe -xclbin ./build_dir.hw.xilinx_u200_xdma_201830_2/logAnalyzer.xclbin -log ./data/access.log -dat ./data/geo.dat -ref ./data/golden.json

Log Analyzer Input Arguments:

.. code-block:: bash

   Usage: test.exe -xclbin <xclbin_name> -log <input log> -data <input geo path> -ref <golden data>
          -xclbin:     the kernel name
          -log   :     input log
          -data  :     input geo path
          -ref   :     golden data

* **Example Output (Step 4)** 

.. code-block:: bash

   ----------------------log analyzer----------------
   DEBUG: found device 0: xilinx_u200_xdma_201830_2
   INFO: initilized context.
   INFO: initilized command queue.
   INFO: created program with binary build_dir.hw.xilinx_u200_xdma_201830_2/logAnalyzer.xclbin
   INFO: built program.
   load log from disk to in-memory buffer
   load geoip database disk to in-memory buffer
   execute log analyzer
   geoIPConvert
   netsLow21 actual use buffer size is 333
   required geo buffer size 1454733
   The log file is partition into 1 slice with max_slice_lnm 102 and  takes 0.006000 ms.
   DEBUG: reEngineKernel has 4 CU(s)
   DEBUG: GeoIP_kernel has 1 CU(s)
   DEBUG: WJ_kernel has 1 CU(s)
   logAnalyzer pipelined, time: 5.401 ms, size: 0 MB, throughput: 0 GB/s
   -----------------------------Finished logAnalyzer pipelined test----------------------------------------------


Profiling
=========

The log analyzer design is validated on an AMD Alveo™ U200 board at 251 MHz frequency. The hardware resource utilizations are listed in the following table.

.. table:: Table 1 Hardware Resources for Log Analyzer
    :align: center
 
    +---------------------+---------+--------+--------+-------+
    | Name                | LUT     | BRAM   | URAM   |  DSP  |
    +---------------------+---------+--------+--------+-------+
    | Platform            | 282591  |  835   |   0    |   16  |
    +---------------------+---------+--------+--------+-------+
    | GeoIP_kernel        |  28802  |   24   |  16    |    8  |
    +---------------------+---------+--------+--------+-------+
    | WJ_kernel           |  32028  |   44   |   0    |    2  |
    +---------------------+---------+--------+--------+-------+
    | reEngineKernel      | 165934  |  264   | 192    |   12  |
    +---------------------+---------+--------+--------+-------+
    |    reEngineKernel_1 |  41412  |   66   |  48    |    3  |
    +---------------------+---------+--------+--------+-------+
    |    reEngineKernel_2 |  41496  |   66   |  48    |    3  |
    +---------------------+---------+--------+--------+-------+
    |    reEngineKernel_3 |  41514  |   66   |  48    |    3  |
    +---------------------+---------+--------+--------+-------+
    |    reEngineKernel_4 |  41512  |   66   |  48    |    3  |
    +---------------------+---------+--------+--------+-------+
    | User Budget         | 899649  | 1325   | 960    | 6824  |
    +---------------------+---------+--------+--------+-------+
    | Used Resources      | 226764  |  332   | 208    |   22  |
    +---------------------+---------+--------+--------+-------+
    | Percentage          | 25.21%  | 25.06% | 21.67% | 0.32% |
    +---------------------+---------+--------+--------+-------+

The performance is shown below.
   This benchmark takes 0.99s to process 1.2 GB data, so its throughput is 1.2 Gb/s.

.. toctree::
   :maxdepth: 1

