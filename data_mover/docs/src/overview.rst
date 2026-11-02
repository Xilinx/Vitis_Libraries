.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _overview:

.. toctree::
   :hidden:

========
Overview
========

The AMD Vitis™ Data Mover Library is an open-source Vitis library for exchanging data between the programmable logic (PL) and AI Engine (AIE).

This library provides the **Programmable 4D Data Mover** and the **Static Data Mover** to help generate kernel designs and improve development efficiency.

The **Programmable 4D Data Mover** has two types of kernels, **4DCuboidRead** and **4DCuboidWrite**, that provide flexible access patterns and high performance between the double-data rate (DDR) memory and AIE.

The **Static Data Mover** has nine types of kernels that connect AIE with DDR, URAM, and block RAM using a contiguous read/write access pattern.
