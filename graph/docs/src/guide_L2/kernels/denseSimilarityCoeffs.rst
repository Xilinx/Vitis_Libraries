.. 
   .. Copyright © 2021–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.


*****************************************************
Internal Design of Dense Similarity with Coefficient
*****************************************************

Interface
===========
The input should be a set of integer vertex list with known size.
The result returns a vertex list with each vertex corresponding to a similarity value.
To optimize the calculation of dense and integer value, the design disables neither float datatype nor sparse mode. Furthermore, the support for Jaccard Similarity is also removed in the kernel so that it saves lots of hardware resources and realizes a design of 2-CU instantiation to get the best performance on the platform of U55C. The design support additional coefficients for each column of weight for better software flexibility.

.. image:: /images/dense_similarity_coefficient_formula.PNG
   :alt:  Formula of Dense Similarity with Coefficient
   :width: 65%
   :align: center

Implementation
============

The detailed algorithm implementation is illustrated below:

.. image:: /images/dense_similarity_coefficient_internal.PNG
   :alt: Diagram of Dense Similarity
   :width: 70%
   :align: center

In the calculation of dense similarity, most of the internal loop size is set by the config variables, so that the reference vertex is aligned with others. The source vertex is initialized by multiplying the value of coefficient. Only integer value can be processed in the kernel, and all the calculation is using LUT arithmetics. In the integer version, the 32-bit input is accumulated by 64-bit registers, and the output float similarity is the division result of two 64-bit integers.
The overall diagram of dense similarity kernel has a insert sort module, which returns the top K number of similarity values.
The maximum number of K is a template number that can be changed by rebuilding the xclbin. The default value of top K is 32.

Profiling and Benchmarks
========================

The kernel is validated on an AMD Alveo |trade| U55C board at 220MHz frequency. 
The hardware resource utilization and benchmark results are shown in the following tables.

.. table:: Table 1 Hardware resources
    :align: center

    +------------------------+--------------+----------------+----------+----------+--------+
    |          Name          |      LUT     |    Register    |   BRAM   |   URAM   |   DSP  |
    +------------------------+--------------+----------------+----------+----------+--------+
    |  denseSimilarityKernel |    262317    |    233100      |    794   |    48    |    9   |
    |  (int + 2CU + Coeffs)  |              |                |          |          |        |
    +------------------------+--------------+----------------+----------+----------+--------+


.. table:: Table 2 Performance comparison of dense graph between TigerGraph on CPU and FPGA
    :align: center
    
    +------------------+----------+----------+-----------------+----------------+------------------------------+
    |                  |          |          |                 |                |  TigerGraph (32 core 512 GB) |
    |     Datasets     |  Vertex  |   Edges  | Similarity Type | FPGA Time / ms +----------------+-------------+
    |                  |          |          |                 |                |   Time / ms    |  Speed up   |
    +------------------+----------+----------+-----------------+----------------+----------------+-------------+
    | Patients(1GB/CU) | 1250000  |   200    |      Cosine     |    7.0         |    585.7       |    83.5     |
    +------------------+----------+----------+-----------------+----------------+----------------+-------------+
    

.. note::
    | 1. Tigergraph running on platform with Intel(R) Xeon(R) CPU E5-2640 v3 @2.600GHz, 32 Threads (16 Core(s)).
    | 2. The uint + float version and integer version have relatively similar performance. 
    | 3. Time unit: ms.

.. toctree::
    :maxdepth: 1

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim: