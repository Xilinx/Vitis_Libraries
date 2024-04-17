.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Zero Coupon Bond, zero, coupon, bond, engine
   :description: A zero-coupon bond is a bond which is purchased at a price below the face value of the bond. It does not pay coupon during the contract period, and repays the face value at the time of maturity.   
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


*************************************************
Internal Design of Zero Coupon Bond Engine
*************************************************


Overview
========
A zero-coupon bond is a bond which is purchased at a price below the face value of the bond. It does not pay coupon during the contract period, and repays the face value at the time of maturity.

Implementation
============
This engine uses the linear interpolation method (:math:`linearInterpolation`) as defined in L1 to calculate the price based on time (the difference between the maturity date and the reference date with the unit in year) and face value. The linear interpolation method implements a one-dimensional linear interpolation. 

Profiling
=========

The hardware resource utilizations are listed in the following table (from AMD Vivado |trade| 18.3 report).

.. table:: Table 1 Hardware resources
    :align: center

    +-----------------+----------+----------+----------+----------+---------+-----------------+
    |  Engine         |   BRAM   |   URAM   |    DSP   |    FF    |   LUT   | clock period(ns)|
    +-----------------+----------+----------+----------+----------+---------+-----------------+
    |  ZCBondEngine   |    0     |    0     |    46    |   12478  |  7997   |       2.580     |
    +-----------------+----------+----------+----------+----------+---------+-----------------+

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
