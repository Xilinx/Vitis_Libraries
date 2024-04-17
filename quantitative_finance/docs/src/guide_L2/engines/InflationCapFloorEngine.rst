.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Inflation CapFloor, inflation
   :description: Inflation option can be cap and floor. An inflation cap (floor) is a financial asset that offers protection against inflation being higher (lower) than a given rate of inflation, and can therefore be used by investors to insure against such inflation outcomes.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


*************************************************
Internal Design of Inflation CapFloor Engine
*************************************************


Overview
========
Inflation option can be cap and floor. An inflation cap (floor) is a financial asset that offers protection against inflation being higher (lower) than a given rate of inflation, 
and can therefore be used by investors to insure against such inflation outcomes.

Implemention
============
The `YoYInflationBlackCapFloorEngine` is year-on-year inflation cap/floor engine based on black formula. The structure of the engine is shown in the following figure:

.. _my-figure1:
.. figure:: /images/inflationEngine.png
    :alt: Figure 1 architecture on FPGA
    :width: 50%
    :align: center

As seen from the figure, the engine mainly contains four functions.

1. function discountFactor: The discount factor is calculated at the corresponding time point.
2. function totalVariance: The total variance of volatility is calculated at the corresponding time point.
3. function yoyRateImpl: The year-on-year forward rate is calculated at the corresponding time point.
4. function blackFormula: The black formula calculates the value of the option based on the results of the three functions mentioned above.

Finally, the addition of the results from each time point is the final price (NPV).

Profiling
=========

The hardware resource utilizations are listed in the following table (from AMD Vivado |trade| 19.1 report).

.. table:: Table 1 Hardware resources
    :align: center

    +------------------------------------+----------+----------+----------+----------+---------+-----------------+
    |  Engine                            |   BRAM   |   URAM   |    DSP   |    FF    |   LUT   | clock period(ns)|
    +------------------------------------+----------+----------+----------+----------+---------+-----------------+
    |  YoYInflationBlackCapFloorEngine   |    0     |    0     |    170   |   33129  |  31999  |       3.210     |
    +------------------------------------+----------+----------+----------+----------+---------+-----------------+

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
