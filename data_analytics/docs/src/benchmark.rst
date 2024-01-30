.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.


.. Project documentation master file, created by
   sphinx-quickstart on Thu Jun 20 14:04:09 2020.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

==========
Benchmark 
==========
    
.. _datasets:

Datasets
-----------

Refer to `Dataset` in Table 1. 


Performance
-----------

For representing the resource utilization in each benchmark, separate the overall utilization into two parts, where P stands for the resource usage in the platform, that is those instantiated in static region of the FPGA card, as well as which K represents those used in kernels (dynamic region). The target device is set to the AMD Alveo™ U280.

.. table:: Table 1 Performance on a FPGA
    :align: center

    +-------------------------------+-----------------------------------------------------------------------------+--------------+----------+-----------------+------------+------------+------------+
    |     Architecture              |      Dataset                                                                |  Latency(ms) |  Timing  |   LUT(P/K)      |  BRAM(P/K) |  URAM(P/K) |  DSP(P/K)  |
    +===============================+=============================================================================+==============+==========+=================+============+============+============+
    | Naive Bayes (U200)            | 999 samples with ten features                                                |    0.519    |  266 MHz  |  185.9K/70.1K   |   345/114  |    0/256   |    10/467|
    +-------------------------------+-----------------------------------------------------------------------------+--------------+----------+-----------------+------------+------------+------------+
    | Support Vector Machine (U250) | 999 samples with 66 features                                                |    0.23      |  300 MHz  |  178.5K/367.0K  |   403/276  |    0/132   |    13/1232|
    +-------------------------------+-----------------------------------------------------------------------------+--------------+----------+-----------------+------------+------------+------------+
    | Log Analyzer Demo (U200)      | 1.2G `access log <http://www.almhuette-raith.at/apache-log/access.log>`_    |    990       |  251 MHz  |  282.6K/226.8K  |   835/332  |    0/208   |    16/22  |
    +-------------------------------+-----------------------------------------------------------------------------+--------------+----------+-----------------+------------+------------+------------+
    | Duplicate Record Match (U50)  | Randomly generate 10,000,000 lines (about 1 GB)                              |    8215560  |  270 MHz  |  135.8K/272.0K  |   180/50   |    0/260   |    4/506 |
    +-------------------------------+-----------------------------------------------------------------------------+--------------+----------+-----------------+------------+------------+------------+

These are details for benchmark result and usage steps:

.. toctree::
   :maxdepth: 1

   Naive Bayes <benchmark/naive_bayes.rst>
   Support Vector Machine <benchmark/svm.rst>
   Log Analyzer <benchmark/log_analyzer.rst>
   Duplicate Record Match <benchmark/dup_match.rst>

Test Overview
--------------

Here are benchmarks of the Vitis Data Analytics Library using the Vitis environment. 

.. _l2_vitis_data_analytics:


* **Download the code**

These data analytics benchmarks can be downloaded from the `vitis libraries <https://github.com/Xilinx/Vitis_Libraries.git>`_ ``main`` branch.

.. code-block:: bash

   git clone https://github.com/Xilinx/Vitis_Libraries.git 
   cd Vitis_Libraries
   git checkout main
   cd data_analytics

* **Set up the environment**

Specify the corresponding Vitis, XRT, and path to the platform repository by running the following commands:

.. code-block:: bash

   source <intstall_path>/installs/lin64/Vitis/2021.1_released/settings64.sh
   source /opt/xilinx/xrt/setup.sh
   export PLATFORM_REPO_PATHS=/opt/xilinx/platforms
