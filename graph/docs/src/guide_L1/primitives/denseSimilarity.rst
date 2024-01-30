.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.


*************************************************
Internal Design of Dense Similarity
*************************************************

Interface
===========
The input should be a set of vertex list with known size.
The result returns a vertex list with each vertex corresponding similarity value.
The config contains several boolean value to control the similarityType (0:Jaccard Similarity, 1:Cosine Similarity), dataType(0:uint32, 1:float, 2:uint64, 3:double, 4:int32, 5:int64). There are two dense similarity primitives are provided in the library. One can perform the computation of both uint and float, and it takes almost twice resource consumption compared with the other dense similarity primitive, which can only support integer as its input. For integer version of dense similarity, there is a design of 2-CU instantiation to get the best performance on the platform of U50.

.. image:: /images/dense_similarity_api.PNG
   :alt: API of Dense Similarity
   :width: 65%
   :align: center

Implementation
============

The detail algorithm implementation is illustrated below:

.. image:: /images/dense_similarity_internal.PNG
   :alt: Diagram of Dense Similarity
   :width: 70%
   :align: center

In the calculation of dense similarity, most of internal loop size is set by the config variables, so that the reference vertex is aligned with others. The calculation is simpler than Sparse Similarity Kernel. In the uint + float version, uint input is transformed to float by primitive internal logic. Then the calculation is done using float arithmetics. In the integer version, the 32-bit input is accumulated by 64-bit registers, and the output float similarity is the division result of two 64-bit integers.
The overall diagram of dense similarity kernel has a insert sort module, which returns the top K number of similarity values.
The maximum number of K is a template number that can be changed by rebuilding the xclbin. The default value of top K is 32.

Profiling and Benchmarks
========================

The Dense Similarity Kernel is validated on AMD Alveo |trade| U50 board at 260MHz frequency. 
The hardware resource utilization and benchmark results are shown in the following tables.

.. table:: Table 1 Hardware resources
    :align: center

    +------------------------+--------------+----------------+----------+----------+--------+
    |          Name          |      LUT     |    Register    |   BRAM   |   URAM   |   DSP  |
    +------------------------+--------------+----------------+----------+----------+--------+
    |  denseSimilarityKernel |    221256    |    329187      |    402   |    16    |   1273 |
    |     (uint + float)     |              |                |          |          |        |
    +------------------------+--------------+----------------+----------+----------+--------+
    |  denseSimilarityKernel |    134446    |    160671      |    402   |    16    |   807  |
    |         (int)          |              |                |          |          |        |
    +------------------------+--------------+----------------+----------+----------+--------+
    |  denseSimilarityKernel |    272521    |    333259      |    618   |    48    |  2364  |
    |       (int + 2CU)      |              |                |          |          |        |
    +------------------------+--------------+----------------+----------+----------+--------+

.. table:: Table 2 Performance comparison of dense graph between TigerGraph on CPU and FPGA
    :align: center
    
    +------------------+----------+----------+-----------------+----------------+------------------------------+
    |                  |          |          |                 |                |  TigerGraph (32 core 512 GB) |
    |     Datasets     |  Vertex  |   Edges  | Similarity Type | FPGA Time / ms +----------------+-------------+
    |                  |          |          |                 |                |   Time / ms    |  Speed up   |
    +------------------+----------+----------+-----------------+----------------+----------------+-------------+
    |   Patients(1 GB) | 1250000  |   200    |      Cosine     |    11.2        |    585.7       |    52.3     |
    +------------------+----------+----------+-----------------+----------------+----------------+-------------+
    

.. Note::
    | 1. Tigergraph running on platform with Intel(R) Xeon(R) CPU E5-2640 v3 @2.600GHz, 32 Threads (16 Core(s)).
    | 2. The uint + float version and integer version have relatively similar performance. 
    | 3. Time unit: ms.

.. toctree::
    :maxdepth: 1


.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim: