.. 
Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
SPDX-License-Identifier: X11

.. _l2_manual_jxl_encoder:

========
JXL Encoder
========

JXL Encoder example resides in ``L2/demos/jxlEnc`` directory. The tutorial provides a step-by-step guide that covers commands for building and running kernel.

Executable Usage
===============

* **Work Directory(Step 1)**

The steps for library download and environment setup can be found in :ref:`l2_vitis_codec`. For getting the design,

.. code-block:: bash

    cd L2/demos/jxlEnc

* **Build kernel(Step 2)**

Run the following make command to build your XCLBIN and host binary targeting a specific device. Note that this process takes a long time, maybe couple of hours.

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