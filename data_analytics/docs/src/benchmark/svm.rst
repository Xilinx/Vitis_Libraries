.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _l2_svm:

======================
Support Vector Machine
======================

Support vector machine (svm) resides in the ``L2/benchmarks/classification/svm`` directory.

Dataset
=======

There are two datasets used in the benchmark:
 1 - PUF (https://archive.ics.uci.edu/ml/datasets/Physical+Unclonable+Functions)

 2 - HIGGS   (https://archive.ics.uci.edu/ml/datasets/HIGGS)

+---------+---------+----------+------------+
| Dataset | Samples | Features | Iterations |
+=========+=========+==========+============+
| 1       | 2000000 |    64    |     20     |
+---------+---------+----------+------------+
| 2       | 5000000 |    28    |     100    |
+---------+---------+----------+------------+

Executable Usage
===============

* **Work Directory (Step 1)**

The steps for library download and environment setup can be found in :ref:`l2_vitis_data_analytics`. For getting the design:

.. code-block:: bash

   cd L2/benchmarks/classification/svm

* **Build kernel(Step 2)**

Run the following make command to build your XCLBIN and host binary targeting a specific device. This process will take a long time, maybe couple of hours.

.. code-block:: bash

   make run TARGET=hw PLATFORM=xilinx_u250_gen3x16_xdma_3_1_202020_1  

* **Run the Kernel (Step 3)**

To get the benchmark results, run the following command.

.. code-block:: bash

   ./build_dir.hw.xilinx_u250_gen3x16_xdma_3_1_202020_1/test_svm.exe -xclbin build_dir.hw.xilinx_u250_gen3x16_xdma_3_1_202020_1/svm_4krnl.xclbin -in ./ml_datasets/1000.csv -trn 999 -ten 100 -fn 64 -itrn 1 -bn 10

Support Vector Machine Input Arguments:

.. code-block:: bash

   Usage: test_svm.exe -xclbin <xclbin_name> -in <input_data> -trn <TBD> -ten <TBD> -fn <TBD> -itrn <TBD> -bn <TBD >
          -xclbin:      the kernel name
          -in    :      input data
          -trn   :      TBD
          -ten   :      TBD
          -fn    :      TBD
          -itrn  :      TBD
          -bn    :      TBD

.. note:: Default arguments are set in the Makefile; you can use other platforms to build and run.

* **Example Output (Step 4)** 

.. code-block:: bash

    --------- SVM Test ---------
    Found Platform
    Platform Name: Xilinx
    Selected Device xilinx_u250_gen3x16_xdma_shell_3_1
    INFO: Importing build_dir.hw.xilinx_u250_gen3x16_xdma_3_1_202020_1/svm_4krnl.xclbin
    ...
    rows num:999
    cols num:65
    creating buf_data
    creating buf_weight
    DDR buffers have been mapped/copy-and-mapped
    Kernel execution timn: 0.23ms
    Decision Tree FPGA times:1 ms
    kernel0:0: -0.00900901
    kernel0:1: -0.0510511
    kernel0:2: -0.033033
    ...
    kernel3:62: -0.013013
    kernel3:63: -0.047047
    
    ------------------------------------------------------------

Profiling
=========

The support vector machine design is validated on an AMD Alveo™ U250 board at a 300 MHz frequency. The hardware resource utilizations are listed in the following table.

.. table:: Table 1 Hardware Resources for Support Vector Machine
    :align: center
    
    +---------------------+----------+---------+---------+--------+
    | Name                | LUT      | BRAM    | URAM    | DSP    |
    +---------------------+----------+---------+---------+--------+
    | Platform            |  178466  |  403    |    0    |    13  |
    +---------------------+----------+---------+---------+--------+
    | SVM                 |  366992  |  276    |  132    |  1232  |
    +---------------------+----------+---------+---------+--------+
    |    SVM_1            |   91725  |   69    |   33    |   308  |
    +---------------------+----------+---------+---------+--------+
    |    SVM_2            |   91671  |   69    |   33    |   308  |
    +---------------------+----------+---------+---------+--------+
    |    SVM_3            |   91815  |   69    |   33    |   308  |
    +---------------------+----------+---------+---------+--------+
    |    SVM_4            |   91781  |   69    |   33    |   308  |
    +---------------------+----------+---------+---------+--------+
    | User Budget         | 1547750  | 2285    | 1280    | 12275  |
    +---------------------+----------+---------+---------+--------+
    | Used Resources      |  366992  |  276    |  132    |  1232  |
    +---------------------+----------+---------+---------+--------+
    | Percentage          |   23.71% |  12.08% |  10.31% | 10.04% |
    +---------------------+----------+---------+---------+--------+
    
The performance is shown below.
    In above test, this design takes 0.23 ms to process 999 samples with 65 features, so its throughput is 1.05 Gb/s.

.. toctree::
   :maxdepth: 1
