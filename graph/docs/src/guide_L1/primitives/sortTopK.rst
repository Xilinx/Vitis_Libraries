.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.


*************************************************
Top K Sort
*************************************************


Overview
========
Top K Sort is a sorting algorithm, which is used to calculate the maximum or the minimum K number of elements in the input stream. The algorithm is quite easy, and you can only get one data per clock due to the design requirements in L2 API. So in the design, use a simple insert sort with adjustable maximum sorting number to get much better performance.

Algorithm
=========
Top K Sort algorithm implementation:

.. code::

    cnt = 0
    tmp[K] = {} //desending array
    for each pair(key, pld) in
        if(cnt < K)
            insert_sort(tmp[], pair)
        else
            if(pair.key > tmp[k].key)
                insert_sort(tmp[], pair)


Implemention
============
The input stream should ensure that it has the same number of key and pld. The internal design is based on inserting sort algorithm.

The algorithm implementation is shown in the following figure:

Figure 1 : Architecture of Top K Sort

.. _my-figure-topKSort:
.. figure:: /images/topKSort.PNG
      :alt: Figure 1 architecture of Top K Sort
      :width: 40%
      :align: center

Profiling
=========

The hardware resource utilizations are listed in the following table.


Table 1 : Hardware resources for Top K Sort with maximum sorting number 64
  
.. table:: Table 1 Hardware resources for Sort Top K (Maximum sortNUM = 64)
    :align: center

    +-------------------+----------+----------+----------+----------+---------+-----------------+
    |      Report       |   BRAM   |   URAM   |    DSP   |    FF    |   LUT   | Frequency(MHz)  |
    +-------------------+----------+----------+----------+----------+---------+-----------------+
    |    top_k_sort     |     0    |     0    |     0    |   5438   |  14061  |      300        |
    +-------------------+----------+----------+----------+----------+---------+-----------------+

    
.. toctree::
   :maxdepth: 1

