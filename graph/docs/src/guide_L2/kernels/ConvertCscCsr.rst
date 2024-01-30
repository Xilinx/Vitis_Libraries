.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.


*************************************************
Internal Design of Convert CSC CSR
*************************************************


Overview
========
ConvertCSCCSR is an algorithm used to transform CSC format input to CSR format input or CSR format input to CSC format input. The algorithm is quite easy, but due to DDR limits, you cannot get one data per clock. So in the design, you use several caches with depth adjustable to get better performance.

Algorithm
============
ConvertCSCCSR algorithm implementation:

.. code::

    for each edge (u, v) in graph   // calculate du degree
        degree(v) += 1
        offset2(v) += 1
    begin = 0
    for node u in graph   
        end = offset1(u)
        for i in (begin, end)
            index = indice1(i)
            index2 = offset2[index]
            offset2[index] += 1
            indice2[index2] = u
        begin = end


Implementation
============
The input matrix should ensure that the following conditions hold:

1. No duplicate edges
2. compressed sparse column/row (CSC/CSR) format

The algorithm implementation is shown in the following figure:

Figure 1 : convert CSC CSR architecture on FPGA

.. _my-figure-ConvertCSCCSR-1:
.. figure:: /images/convertCsr2Csc.png
      :alt: Figure 1 Convert CSC CSR architecture on FPGA
      :width: 80%
      :align: center

As seen from the figure:

1. Firstly, call the Module `calculate degree` to generate the transfer offset array.
2. By using the input offset and indice arrays and also the calculated new offset array, generate the new indice array.

Profiling
=========

The hardware resource utilizations are listed in the following table.


Table 1 : Hardware resources for ConvertCSCCSR with cache
  
.. table:: Table 1 Hardware resources for ConvertCSCCSR with cache (depth 1)
    :align: center

    +-------------------+----------+----------+----------+----------+---------+-----------------+
    |    Kernel         |   BRAM   |   URAM   |    DSP   |    FF    |   LUT   | Frequency(MHz)  |
    +-------------------+----------+----------+----------+----------+---------+-----------------+
    | kernel_pagerank_0 |    413   |     0    |     7    |  295330  |  207754 |      300        |
    +-------------------+----------+----------+----------+----------+---------+-----------------+

Figure 2 : Cache depth's influence to ConvertCSCCSR acceleration

.. _my-figure-ConvertCSCCSR-2:
.. figure:: /images/ConvertCsrCsc_AR.png
      :alt: Figure 2 Cache depth's influence to ConvertCSCCSR acceleration
      :width: 50%
      :align: center


.. Note::
    | 1. depth 1, depth 32, depth 1k, they use LUTRAM only, in compare with resources of depth 1, only LUT and FF changes.
    | 2. depth 4k, depth 32k, they use URAM, in compare with resources of depth 1, the URAM utilization is the major difference.
    | 3. HW Frequency: depth 1 (300MHz), depth 32 (300MHz), depth 1k (275.6MHz), depth 4k (300MHz), depth 32k (275.7MHz)

.. toctree::
   :maxdepth: 1

