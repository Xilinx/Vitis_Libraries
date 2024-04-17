.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: consumer price index, CPI
   :description: A consumer price index (CPI) Cap/Floor is a call/put on the CPI.  
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


*************************************************
Internal Design of CPI CapFloor Engine
*************************************************


Overview
========
A consumer price index (CPI) Cap/Floor is a call/put on the CPI. 

Implemention
============
This engine uses the linear interpolation method (:math:`linearInterpolation2D`) as defined in L1 to calculate the price based on time (the difference between the maturity date and the reference date with unit in year) and strike rate. The linear interpolation method implements a two-dimensional linear interpolation. 

Profiling
=========

The hardware resource utilizations are listed in the following table (from AMD Vivado |trade| 18.3 report).

.. table:: Table 1 Hardware resources
    :align: center

    +----------------------+----------+----------+----------+----------+---------+-----------------+
    |  Engine              |   BRAM   |   URAM   |    DSP   |    FF    |   LUT   | clock period(ns)|
    +----------------------+----------+----------+----------+----------+---------+-----------------+
    |  CPICapFloorEngine   |    0     |    0     |    22    |   11385  |  7625   |       2.966     |
    +----------------------+----------+----------+----------+----------+---------+-----------------+

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
