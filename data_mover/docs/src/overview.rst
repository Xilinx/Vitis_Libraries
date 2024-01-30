.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _overview:

.. toctree::
   :hidden:

========
Overview
========

The AMD Vitis™ Data Mover Library is an open-sourced Vitis library for exchanging data between the programmable logic (PL) and AI Engine (AIE).

This library provides the **Programmable 4D Data-Mover** and **Static Data-Mover** to help generate a kernel design and improve development efficiency.

The **Programmable 4D Data-Mover** has two types of kernels, **4DCuboidRead** and **4DCuboidWrite**, which provide both flexible access patterns and keep high performance between the double-data rate (DDR) and AIE.

**Static Data-Mover** has nine types of kernels which help AIE connect with DDR/URAM and block RAM. Their access pattern is a simple continously read/write.