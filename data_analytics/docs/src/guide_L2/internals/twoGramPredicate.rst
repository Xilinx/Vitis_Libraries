.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

******************
Two Gram Predicate
******************

Overview
========

Two Gram Predicate (TGP) is a search of the inverted index with a term of two characters. For a dataset that established an inverted index, it can find the matching ID in each record in the inverted index.

Implementation
================

The TGP kernel design is show in the following figure:

.. _my-figure1:
.. figure:: /images/text/twoGramPredicate.png
    :alt: Figure 1 TGP Kenrel architecture on FPGA
    :width: 30%
    :align: center

The functions of each module in the figure is:

- `preProcess`: Delete special characters and use 6-bit encoding characters. The encoding rules are: 0~9 <-> '0'~'9', 10~35 <-> 'a/A'~'z/Z', 36 -> ' ' & '\n'.
- `twoGram`: Split field to terms according to 2-Gram, and one term is two characters.
- `insertSort`: Sort terms using insertion sort algorithm.
- `getIDFTFAddr`: Merge the same term, and use term as the address to get term frequency (TF) value and the address of inverted document frequency (IDF) value in block RAM. Both TF vlaue and IDF value are provided by the inverted index.
- `weighted Union`: The module is a key design, and its detailed information is shown in the following figure.

.. _my-figure2:
.. figure:: /images/text/weightedUnion.png
    :alt: Figure 2 Key Design of weighted Union
    :width: 150%
    :align: center

The function of each module of `weighted Union` in the figure is:

- `distData`: According to the order of term, distribute the TF value to the corresponding channel. The core design is to distribute data to each channel cyclically through the `write_nb` operation and the subsequent `mBackPressure` module.
- `mBackPressure`: Cooperate with the `distData` module to realize back pressure control.
- `mergeTree`: Combine CH channels into one, and its bottom layer is implemented by `mergeSum`. The function of `mergeSum` is to add values with the same ID.
- `byValue`: Get the first index ID that exceeds the threshold.

For more details, refer to the source code.

Resource Utilization
====================
 
The TGP Kernel is validated on an AMD Alveo™ U50 card.
The hardware resources utilization and frequency are listed in the table above (not include Platform).

+----------------+---------+---------+--------+--------+-------+---------+
|      Name      |   LUT   |   REG   |  BRAM  | URAM   | DSP   |   Freq  |
+----------------+---------+---------+--------+--------+-------+---------+
|                | 135,616 | 18,8261 |   25   |  130   |  253  |         |
|   TGP_Kernel   +---------+---------+--------+--------+-------+ 294 MHz |
|                |  17.90% |  11.81% | 2.14%  | 20.31% | 4.26% |         |
+----------------+---------+---------+--------+--------+-------+---------+

.. toctree::
   :maxdepth: 1
