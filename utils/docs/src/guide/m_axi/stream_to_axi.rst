.. 
  .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _guide-stream_to_axi:

********************************
Internals of streamToAxi
********************************

.. toctree::
   :hidden:
   :maxdepth: 2

This document describes the structure and execution of streamToAxi,
implemented as :ref:`streamToAxi <cid-xf::common::utils_hw::streamToAxi>` function.

This function is designed for writing data into AXI master in burst mode.

.. _my-figure-strem_to_axi:
.. figure:: /images/stream_to_axi.png
    :alt: bit field
    :width: 80%
    :align: center

    stream to axi workflow

.. CAUTION::
    Applicable conditions:

    1. AXI port width should be a multiple of stream width.

    2. countForBurst: Converts stream width from _WStrm into _WAxi and counts the burst number.

    3. burstWrite: It reads the number of burst from stream, then burst writes to the axi port in dataflow.

This primitive performs streamToAxi in two modules working simultaneously.

The implementation of the two modules is shown as follows:

.. _my-figure-param:
.. figure:: /images/burst_to_write.png
    :alt: bit field
    :width: 80%
    :align: center

    countForBurst imprementation details

where ``N = _WAxi/_WStrm`` , ``not enough one axi`` is the state of ``counter_for_axi < N`` and it would be instead of 0.
``not enough one burst`` is the state of ``counter_for_burst < NBurst`` and it acts as a burst to write.

This ``streamToAxi`` primitive has only one port for axi ptr and one port for stream output.

