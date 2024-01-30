.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: dynamic-filter
   :description: Describes the structure and execution of the dynamic filter module.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _guide-dynamic_filter:

*********************************
Internals of Dynamic-Filter
*********************************

.. toctree::
   :hidden:
   :maxdepth: 1

Internal Structure
==================

The following figure illustrates the internal of the dynamic filter. On the left, a group of range-checkers compares each column data with the upper and lower bounds specified by two constants and two operators; while on the right, each pair of columns is assigned to a comparator. The final condition is yield by looking into a true-table using an address consisting of bits from these two parts.

.. image:: /images/dynamic_filter.png
   :alt: Dynamic Filter Structure
   :align: center

Limitations
===========

Currently, up to **four** condition columns of **integral types** are supported. Wrappers for less input columns are provided; the configuration structure **remains the same as the four-input version**.

.. CAUTION::
   Filter operator has signed or unsigned version; check ``enum FilterOp`` in ``enums.h`` for details.

Generating Config Bits
======================

Currently, there is no expression-string to the config bits compiler yet. For generating the raw config bits, see the demo project in ``L1/demos/q6_mod/host/filter_test.cpp``.

The layout of the configuration bits is illustrated in the following figure. As the figures shows, the intermediates are always aligned to 32 bit boundaries and comes in *little-endian* (in the figure, the intermediates are 48 bit wide, and thus occupies one and a half rows).

.. image:: /images/dynamic_filter_config.png
   :alt: Dynamic Filter Configuration Bits Layout
   :align: center