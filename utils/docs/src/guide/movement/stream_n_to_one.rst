.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _guide-stream_n_to_one:

*****************************************
Internals of streamNToOne
*****************************************

The :ref:`streamNToOne <cid-xf::common::utils_hw::streamNToOne>` API
is designed for collecting data from multiple processor units.
Three different algorithms have been implemented, ``RoundRobinT``,
``LoadBalanceT``, and ``TagSelectT``.

To ensure the throughput, it is common to pass a vector of elements in the
FPGA data paths. If the
data elements are passed in the form of ``ap_uint``, ``streamNToOne`` supports the element vector output.
It also offers an overload for generic template type for non-vector output.

.. contents::
   :depth: 2

Round-Robin
===========

The round-robin algorithm collects elements from input streams in circular
order, starting from the output stream with index 0.

Generic Type
~~~~~~~~~~~~

With a generic type input, the function dispatches one element per cycle.
This mode works best for sharing the multi-cycle processing work across
an array of units.

.. image:: /images/stream_n_to_one_round_robin_type.png
   :alt: structure of round-robin collect
   :width: 80%
   :align: center

Vector Output
~~~~~~~~~~~~~

The design of the primitive includes three modules:

1. fetch: Attempts to read data from the `n` input streams.

2. vectorize: Inner buffers as wide as the least common multiple of ``N * Win``
   and ``Wout`` are used to combine the inputs into vectors.

3. emit: Reads vectorized data and emits to output stream.

.. image:: /images/stream_n_to_one_round_robin_detail.png
   :alt: structure of vectorized round-robin collection
   :width: 100%
   :align: center

   However, the current implementation has the following limitations:

   * It uses a wide ``ap_uint`` as an internal buffer. The buffer is as wide as
     the least common multiple (LCM) of input width and total output width.
     The width is limited by ``AP_INT_MAX_W``, which defaults to 1024.
   * This library tries to override ``AP_INT_MAX_W`` to 4096. Ensure that ``ap_int.h`` is not included before the library
     headers.
   * Too large ``AP_INT_MAX_W`` significantly slows down HLS synthesis.


Load-Balancing
==============

The load-balancing algorithm does not keep a fixed order in collection. Instead, it skips predecessors that cannot be read, and tries to feed the output as much as possible.

Generic Type
~~~~~~~~~~~~

.. image:: /images/stream_n_to_one_load_balance_type.png
   :alt: structure of load-balance collection
   :width: 80%
   :align: center


Vector Output
~~~~~~~~~~~~~~

The design of the primitive includes three modules:

1. fetch: Attempts to read data from the `n` input streams.

2. vectorize: Inner buffers as wide as the least common multiple of  ``N * Win``
   and ``Wout`` are used to combine the inputs into vectors.

3. emit: Reads vectorized data and emit to output stream.

.. image:: /images/stream_n_to_one_load_balance_detail.png
   :alt: structure of vectorized load-balance collection
   :width: 100%
   :align: center

   However, the current implementation has the following limitations:

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

This algorithm collects data elements according to provided tags.
The tags are used as index of input streams, and it is expected that
each input element is accompanied by a tag.

.. image:: /images/stream_n_to_one_tag_select_type.png
   :alt: structure of tag-select collect
   :width: 80%
   :align: center

