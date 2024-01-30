.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.
.. _guide-stream_discard:

*****************************************
Internals of streamDiscard
*****************************************

.. toctree::
   :hidden:
   :maxdepth: 2

The :ref:`streamDiscard <cid-xf::common::utils_hw::streamDiscard>` module
works as a sink of stream(s).
It basically reads all inputs and discard them.
You can use the :ref:`streamOneToN <guide-stream_one_to_n>`'s ``TagSelectT``
option to route data to this module to do dynamic data removal.

Three variants have been implemented, handling normal stream or synchronized
streams.

.. image:: /images/stream_discard_single.png
   :alt: discarding a singe stream structure
   :width: 80%
   :align: center

.. image:: /images/stream_discard_array.png
   :alt: discarding streams structure
   :width: 80%
   :align: center

.. image:: /images/stream_discard_share.png
   :alt: discarding streams sharing end-flag structure
   :width: 80%
   :align: center

The code implementation is a simple ``while`` loop.
