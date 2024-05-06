.. 
Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
SPDX-License-Identifier: X11

.. _l2_kernel_jxl_encoder:

===========
JXL Encoder
===========

JXL Encoder example resides in ``L2/demos/jxlEnc`` directory. The tutorial provides a step-by-step guide that covers commands for building and running kernel.

Overview
========
JXL Encoder demos contain 3 kernels which show acceleration cases on different parts of JPEG-XL encoder. JxlEnc_lossy_enc_compute kernel mainly responsible for accelerating AC and DC generation and processing. The input is XYB image data and other componets such as maskfield, raw quantfield and aq maps. The output is AC and DC coefficients, and block order and strategy and quantfield for next steps. JxlEnc_ans_initHistogram and JxlEnc_ans_clusterHistogram are two parts of ANS encoding, accelerating on these two kernels responsible for generation of ACtokens and histograms. The internel block design is show as below.

The design of the JxlEnc_lossy_enc_compute kernel is as follows:

.. image:: /images/JxlEnc_lossy_enc_compute_blockDesign.png
   :alt: Block Design of JxlEnc_lossy_enc_compute
   :align: center

LoadData is responsible for load host data to internal stream and pass to next step. Then, a parallel computing of DCT8x8, DCT16x16 and DCT32x32 is processed by VarDCT and the result is sending to acs_heuritic which further compute ac strategy for each image block. CFL is responsible for color correlation of YtoX and YtoB and also pass quantfield and acs to next module. In Compute_CoeffAC, ac coefficients are generated and then ouput to AXI writeout. All order of image blocks are computed after dataflow processing of AC and DC coefficients, its' result are then send to AXI writeout.   

The design of the JxlEnc_ans_initHistogram and JxlEnc_ans_clusterHistogram is as follows:

.. image:: /images/JxlEnc_init_cluster_blockDesign.png
   :alt: Block Design of JxlEnc_ans_initHistogram AND JxlEnc_ans_initHistogram
   :align: center

Kernel JxlEnc_ans_initHistogram and JxlEnc_ans_clusterHistogram are designed for accelerating ANS encoding. The JxlEnc_ans_initHistogram is processed within dataflow and parallely doing AC Tokenize and Histogram initiation. For JxlEnc_ans_clusterHistogram, it is processed in pipeline acceleration and generates all histograms for post-processing in JPEG-XL computing flow.

Executable Usage
===============

* **Work Directory(Step 1)**

The steps for library download and environment setup can be found in :ref:`l2_vitis_codec`. For getting the design,

.. code-block:: bash

    cd L2/demos/jxlEnc

* **Build kernel(Step 2)**

Run the following make command to build your XCLBIN and host binary targeting a specific device. Please be noticed that this process will take a long time, maybe couple of hours.

.. code-block:: bash

    make run TARGET=hw 

* **Run kernel(Step 3)**

To get the benchmark results, please run the following command.

.. code-block:: bash

    PATH_TO_BUILD/host.exe --xclbin PATH_TO_BUILD/jxlEnc.xclbin PNGFilePath JXLFilePath

Note: "PATH_TO_BUILD" is decided by your chosen "PLATFORM=" when running hw build, Default arguments are set in Makefile.   

JXL Encoder Input Arguments:

.. code-block:: bash

   Usage: host.exe -[-xclbin]
          --xclbin:     the kernel name
          PNGFilePath:  the path to the input *.PNG
          JXLFilePath:  the path to the output *.jxl

Note: Default arguments are set in Makefile, you can use other :ref:`pictures` listed in the table.

Profiling
=========

The hardware resource utilizations are listed in the following table.
Different tool versions may result slightly different resource.


.. table:: Table 1 IP resources for JXL encoder
    :align: center

    +---------------------------------+----------+----------+----------+----------+---------+
    |               IP                |   BRAM   |   URAM   |    DSP   |    FF    |   LUT   |
    +---------------------------------+----------+----------+----------+----------+---------+
    |    JxlEnc_lossy_enc_compute     |    364   |    53    |    498   |   145111 |  121741 |
    +---------------------------------+----------+----------+----------+----------+---------+
    |    JxlEnc_ans_clusterHistogram  |    70    |    28    |    51    |   60744  |  38507  |
    +---------------------------------+----------+----------+----------+----------+---------+
    |    JxlEnc_ans_initHistogram     |    150   |    41    |    95    |   64710  |  39289  |
    +---------------------------------+----------+----------+----------+----------+---------+

Result
======

.. table:: Table JxlEnc_lossy_enc_compute Encoder Performance
    :align: center
      
    +-------------------+---------------+------------+--------------------+      
    |       Image       |      Size     |  Time(ms)  |  Throughput(MP/s)  |   
    +-------------------+---------------+------------+--------------------+   
    |  lena_c_512.png   |    512x512    |    3.63    |        72.21       |       
    +-------------------+---------------+------------+--------------------+   
    |  hq_1024x1024.png |   1024x1024   |    13.06   |        80.29       |      
    +-------------------+---------------+------------+--------------------+   
    |  hq_2Kx2K.png     |   2048x2048   |    50.33   |        83.34       |     
    +-------------------+---------------+------------+--------------------+      

.. table:: Table JxlEnc_ans_clusterHistogram Encoder Performance
    :align: center
 
    +-------------------+---------------+------------+--------------------+
    |       Image       |      Size     |  Time(ms)  |  Throughput(MP/s)  |
    +-------------------+---------------+------------+--------------------+
    |  lena_c_512.png   |    512x512    |    4.6     |        56.98       |     
    +-------------------+---------------+------------+--------------------+
    |  hq_1024x1024.png |   1024x1024   |    14.6    |        71.82       |    
    +-------------------+---------------+------------+--------------------+
    |  hq_2Kx2K.png     |   2048x2048   |    41.13   |        101.97      | 
    +-------------------+---------------+------------+--------------------+

.. table:: JxlEnc_ans_initHistogram Encoder Performance
    :align: center

    +-------------------+---------------+-------------+--------------------+
    |       Image       |      Size     |   Time(ms)  |  Throughput(MP/s)  |
    +-------------------+---------------+-------------+--------------------+
    |  lena_c_512.png   |    512x512    |    6.07     |        43.19       |     
    +-------------------+---------------+-------------+--------------------+
    |  hq_1024x1024.png |   1024x1024   |    18.03    |        58.16       |    
    +-------------------+---------------+-------------+--------------------+
    |  hq_2Kx2K.png     |   2048x2048   |    79.30    |        52.89       |  
    +-------------------+---------------+-------------+--------------------+

.. toctree::
   :maxdepth: 1

.. Copyright © 2020–2023 Advanced Micro Devices, Inc
.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.