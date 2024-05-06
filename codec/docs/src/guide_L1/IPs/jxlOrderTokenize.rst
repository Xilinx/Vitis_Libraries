.. 
Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
SPDX-License-Identifier: X11

***********************************
Internal Design of Order Tokenize
***********************************

Overview
===========

This API is a submodule of the jpegXL encoder. It is an optimized implementation of the Vitis HLS design methodology. This API is able to construct tokens of the orders of the ac coefficients of jpegXL. The supported AC strategy includes 8x8, IDENTITY, 16x16, and 32x32 DCT. The input of this API is a set of orders of coefficients to be encoded under all the supported strategies. And the output is a number of tokens which can be encoded by the following ANS encoder.

Implemention
============

The detail algorithm implemention is illustrated as below:

.. image:: /images/orderTokenize.png
   :alt: desigh of Order Tokenize
   :width: 60%
   :align: center

As it is shown in the aboved pictures, the whole orderTokenize have 4 functions and dataflow between these functions.

* scanStrategy: the function gets ac strategy from the input "used_orders" and compute the size according to ac strategy type. 

* loadZigzag: the function lookups the created table to get zigzag according to its input orders stream.

* updateLehmer: the function computes Lehmer according to its input zigzag stream.

* updateToken: the function computes token value and context according to its lehmer stream.

Profiling 
============

The Post-Synthesis Resource usage are shown in the table1 below.
The Order Tokenize C/RTL co-simulation on CPU, and the result is based it in table2.   

.. table:: Table 1 Post-Synthesis Resource usage
    :align: center

    +------------------+-----------+-----------+----------+----------+--------+
    |       Name       |    LUT    |  Register |   BRAM   |   URAM   |   DSP  |
    +------------------+-----------+-----------+----------+----------+--------+
    |  orderTokenize   |   2581    |   1835    |     6    |    0     |    0   |
    +------------------+-----------+-----------+----------+----------+--------+


.. table:: Table 2 Comparison between orderTokenize on CPU and Cosim
    :align: center
    
    +------------------+----------+-----------+------------+------------+
    |      Image       |   Size   |    Cpu    |   Cosim    |   Speed    |
    +------------------+----------+-----------+------------+------------+
    |      t0.png      |   960    |   0.139   |   0.058    |    2.4x    |
    +------------------+----------+-----------+------------+------------+
    | Ali_512x512.png  |   960    |   0.233   |   0.058    |    4.0x    |
    +------------------+----------+-----------+------------+------------+
    |   853x640.png    |   1152   |   0.152   |   0.069    |    2.2x    |
    +------------------+----------+-----------+------------+------------+
    |   hq_2Kx2K.png   |   1152   |   0.120   |   0.069    |    1.7x    |
    +------------------+----------+-----------+------------+------------+

.. note::
    | 1. orderTokenize running on platform with Intel(R) Xeon(R) CPU E5-2690 v4 @ 2.60GHz, 28 Threads (14 Core(s)).
    | 2. Time unit: ms.

.. toctree::
    :maxdepth: 1
	
.. Copyright © 2020–2023 Advanced Micro Devices, Inc
.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.
