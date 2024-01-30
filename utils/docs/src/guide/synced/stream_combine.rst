.. 
 
.. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _guide-stream_combine:

*****************************************
Internals of streamCombine
*****************************************

.. toctree::
   :hidden:
   :maxdepth: 3

The :ref:`streamCombine <cid-xf::common::utils_hw::streamCombine>` function
is designed for packing multiple elements of same width into a vector.

This module offers two static configurations: using the data from LSB or MSB.
With an LSB option, the element at LSB is obtained from input stream with 0 index,
while with an MSB option, the element at MSB is set using input with 0 index.

As some storage structures in FPGA are bounded to fixed width or width of power
of two, paddings might be necessary in the combined vector.
These padding bits are added with zeros, as illustrated below:

.. image:: /images/stream_combine_lsb.png
   :alt: combination n streams to one from LSB Structure
   :width: 80%
   :align: center

.. image:: /images/stream_combine_msb.png
   :alt: combination n streams to one from MSB Structure
   :width: 80%
   :align: center

Internally, this module is implemented with a simple loop which initiation
interval (II) is equal to 1.
This means that in each cycle, a vector is yielded using a set of elements.

.. ATTENTION::
   This module expects the width of output stream to be no less than total of
   input streams. To perform collection from multiple streams, consider the
   :ref:`streamNToOne <guide-stream_n_to_one>` module.

