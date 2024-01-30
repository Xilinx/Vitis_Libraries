.. 
 
.. Copyright © 2019-2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _guide-stream_split:

*****************************************
Internals of streamSplit
*****************************************

.. toctree::
   :hidden:
   :maxdepth: 3

The :ref:`streamSplit <cid-xf::common::utils_hw::streamSplit>` is designed
for splitting a wide stream into multiple narrow ones, as it is common to
combine several data elements of the same type as a vector and pass them
together in FPGA data paths.

This module offers two static configurations: using the data from LSB or MSB.
With an LSB option, the element at LSB is sent to output stream with 0 index,
while with an MSB option, the element at MSB is sent to output with 0 index.

As some storage structures in FPGA are bounded to fixed width or width of power
of two, paddings might be necessary in the combined vector.
These padding bits are discarded during splitting, as illustrated below:

.. image:: /images/stream_split_lsb.png
   :alt: one stream to n distribution on MSB Structure
   :width: 80%
   :align: center

.. image:: /images/stream_split_msb.png
   :alt: one stream to n distribution on MSB Structure
   :width: 80%
   :align: center

Internally, this module is implemented with a simple loop which initiation
interval (II) is equal to 1.
This means that in each cycle, a vector is split into a set of elements.

.. ATTENTION::
   This module expects the width of input stream to be no less than the total of
   output streams. To perform distribution from a vectors of elements to
   multiple streams, use the
   :ref:`streamOneToN <guide-stream_one_to_n>` module.

