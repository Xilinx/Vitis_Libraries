.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _l2_naive_baye:

===========
Naive Bayes
===========

Naive Bayes resides in the ``L2/benchmarks/classification/naive_bayes`` directory.

Dataset
=======

There are three dataset used in the benchmark:

 1 - RCV1 (https://scikit-learn.org/0.18/datasets/rcv1.html)

 2 - webspam (https://chato.cl/webspam/datasets/uk2007/)

 3 - news20 (https://scikit-learn.org/0.19/datasets/twenty_newsgroups.html)

+---------+---------+---------+----------+
| Dataset | Examples | Classes | Features|
+=========+=========+=========+==========+
| RCV1    | 697614  |   2     |  47236   |
+---------+---------+---------+----------+
| webspam | 350000  |   2     |  254     |
+---------+---------+---------+----------+
| news20  | 19928   |   20    |  62061   |
+---------+---------+---------+----------+

Executable Usage
===============

* **Work Directory (Step 1)**

The steps for library download and environment setup can be found in :ref:`l2_vitis_data_analytics`. For getting the design:

.. code-block:: bash

   cd L2/benchmarks/classification/naive_bayes

* **Build the Kernel (Step 2)**

Run the following make command to build your XCLBIN and host binary targeting a specific device. This process will take a long time, maybe couple of hours.

.. code-block:: bash

   make run TARGET=hw PLATFORM=xilinx_u200_xdma_201830_2 

* **Run the Kernel (Step 3)**

To get the benchmark results, run the following command:

.. code-block:: bash

   ./build_dir.hw.xilinx_u200_xdma_201830_2/test_nb.exe -xclbin build_dir.hw.xilinx_u200_xdma_201830_2/naiveBayesTrain_kernel.xclbin ./data/test.dat -g ./data/test_g.dat -c 10 -t 13107

Naive Bayes Input Arguments:

.. code-block:: bash

   Usage: test_nb.exe -xclbin <xclbin_name> -in <input_data> -g <golden_data> -c <number of class> -t <number of feature>
          -xclbin:      the kernel name
          -in    :      input data
          -g     :      golden data
          -c     :      number of class
          -t     :      number of feature

.. note:: Default arguments are set in the Makefile; you can use other platforms to build and run.

* **Example Output (Step 4)** 

.. code-block:: bash

    ---------------------Multinomial Training Test of Naive Bayes-----------------
    Found Platform
    Platform Name: Xilinx
    Found Device=xilinx_u200_xdma_201830_2
    INFO: Importing build_dir.hw.xilinx_u200_xdma_201830_2/naiveBayesTrain_kernel.xclbin
    Loading: 'build_dir.hw.xilinx_u200_xdma_201830_2/naiveBayesTrain_kernel.xclbin'
    kernel has been created
    kernel start------
    kernel end------
    Total Execution time 17.381ms
    
    Start Profiling...
    Write DDR Execution time 0.108582ms
    Kernel Execution time 0.519421ms
    Read DDR Execution time 0.03953ms
    Total Execution time 0.667533ms
    ============================================================
    
    Prior probability:
    -2.34341 -2.38597 -2.30259 -2.43042 -2.20727 -2.36446 -2.22562 -2.30259 
    -2.27303 -2.21641 0 0 0 0 0 0 
    Check pass.
    
    ------------------------------------------------------------

Profiling
=========

The Naive Bayes design is validated on an AMD Alveo™ U200 board at 266 MHz frequency. The hardware resource utilizations are listed in the following table.

.. table:: Table 1 Hardware Resources for Naive Bayes
    :align: center
    
    +--------------------------+---------------+-----------+-----------+----------+
    |           Name           |       LUT     |    BRAM   |    URAM   |    DSP   |
    +--------------------------+---------------+-----------+-----------+----------+
    |         Platform         |     185929    |    345    |    0      |    10    |
    +--------------------------+---------------+-----------+-----------+----------+
    |  naiveBayesTrain_kernel  |     70058     |    114    |    256    |    467   |
    +--------------------------+---------------+-----------+-----------+----------+
    |        User Budget       |     996311    |    1815   |    960    |    6830  |
    +--------------------------+---------------+-----------+-----------+----------+
    |        Percentage        |     7.03%     |    6.28%  |    26.67% |    6.84% |
    +--------------------------+---------------+-----------+-----------+----------+

The performance is shown below.
    This benchmark takes 0.519421 ms to train 999 samples with 10 features, so the throughput is 73.37 Mb/s.


.. toctree::
   :maxdepth: 1

