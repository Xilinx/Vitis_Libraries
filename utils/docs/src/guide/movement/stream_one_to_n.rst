.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _guide-stream_one_to_n:

*****************************************
Internals of streamOneToN
*****************************************

The :ref:`streamOneToN <cid-xf::common::utils_hw::streamOneToN>` API
is designed for distributing data from one source to multiple processor units.
Three different algorithms have been implemented, ``RoundRobinT``,
``LoadBalanceT``, and ``TagSelectT``.

To ensure the throughput, it is common to pass a vector of elements in
FPGA data paths. If the
data elements are passed in the form of ``ap_uint``, ``streamOneToN`` supports an element vector input.
It also offers an overload for generic template type for non-vector input.

.. contents::
   :depth: 2

Round-Robin
===========

The round-robin algorithm distributes elements to output streams in a circular
order, starting from the output stream with index 0.

Generic Type
~~~~~~~~~~~~

With generic type input, the function dispatches one element per cycle.
This mode works best for sharing the multi-cycle processing work across
an array of units.

.. image:: /images/stream_one_to_n_round_robin_type.png
   :alt: one-to-n round-robin
   :width: 80%
   :align: center

Vector Input
~~~~~~~~~~~~

With input casted to a long ``ap_uint`` vector, higher input rate can be done.
This implementation consists of two dataflow processes working in parallel.
The first one breaks the vector into a ping-pong buffer,
while the second one reads from the buffers and schedules output in a
round-robin order.

.. image:: /images/stream_one_to_n_round_robin_detail.png
   :alt:  design details of n streams to one distribution on round robin
   :width: 100%
   :align: center

The ping-pong buffers are implemented as two ``ap_uint`` of width as least
common multiple (LCM) of input width and total output stream width.
This imposes a limitation, as the LCM should be no more than
``AP_INT_MAX_W``, which is default to 1024 in HLS.

.. CAUTION::
   Though ``AP_INT_MAX_W`` can be set to larger values, it might slow down HLS
   synthesis. The macro must be
   set before first inclusion of ``ap_int.h`` header to effectively override ``AP_INT_MAX_W``.

   This library tries to override ``AP_INT_MAX_W`` to 4096, but it is only
   effective when ``ap_int.h`` is not included before utility library
   headers.

Load-Balancing
==============

The load-balancing algorithm does not keep a fixed order in dispatching. Instead, it skips successors that cannot read, and tries to feed as much
as possible to outputs.

Generic Type
~~~~~~~~~~~~

.. image:: /images/stream_one_to_n_load_balance_type.png
   :alt: stream_one_to_n distribution on load balance Structure
   :width: 80%
   :align: center


Vector Input
~~~~~~~~~~~~

The design of the primitive includes three modules:

1. read: Reads data from the input stream then output data by one stream whose
   width is ``lcm(Win, N * Wout)`` bits.
   Here, the least common multiple of  ``Win`` and ``N * Wout`` is the inner
   buffer size to solve the different input width and output width.

2. reduce: Splits the large width to an array of ``N`` elements of ``Wout`` bits.

3. distribute: Reads the array of elements, and distributes them to output streams that
   are not yet full.

.. image:: /images/stream_one_to_n_load_balance_detail.png
   :alt:  design details of n streams to one distribution on load balance
   :width: 100%
   :align: center

.. ATTENTION::
   Current implementation has the following limitations:

   * It uses a wide ``ap_uint`` as an internal buffer. The buffer is as wide as
     the least common multiple (LCM) of input width and total output width.
     The width is limited by ``AP_INT_MAX_W``, which defaults to 1024.
   * This library tries to override ``AP_INT_MAX_W`` to 4096. Ensure that ``ap_int.h`` is not included before the library
     headers.
   * Too large ``AP_INT_MAX_W`` significantly slows down HLS synthesis.

.. IMPORTANT::
   The depth of output streams must be no less than four due to an internal delay.

Tag-Select
==========

This algorithm dispatches data elements according to provided tags.
The tags are used as index of output streams, and it is expected that
each input element is accompanied by a tag.

.. image:: /images/stream_one_to_n_tag_select_type.png
   :alt: one stream to n distribution on tag Structure
   :width: 80%
   :align: center

