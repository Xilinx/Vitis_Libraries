..
   Copyright © 2019–2024 Advanced Micro Devices, Inc
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _FFT_2D:

============
FFT IFFT 2D
============

This library element implements a 2D FFT/IFFT function. It has configurable point sizes, data types, and
sizes separately for the two dimensions. It also has a configurable twiddle type, rounding and saturation modes, scaling (as a shift), cascade length, and twiddle mode.

Entry Point
===========

The graph entry point is the following:

.. code-block::

    xf::dsp::aie::fft::two_d::fft_ifft_2d_graph

Device Support
==============

The 2D FFT/IFFT supports AIE-ML devices only.


Supported Types
===============

The input data types to the 2D FFT are controlled by the ``TT_DATA_D1`` and ``TT_DATA_D2`` template parameters, see :ref:`API_REFERENCE`.

Template Parameters
===================

To see details on the template parameters for the 2D FFT/IFFT, see :ref:`API_REFERENCE`.

Access Functions
================

To see details on the access functions for the 2D FFT/IFFT, see :ref:`API_REFERENCE`.

Ports
=====
To see details on the ports for the 2D FFT/IFFT, see :ref:`API_REFERENCE`.


Code Example
============
.. literalinclude:: ../../../../L2/examples/docs_example_fft_2d/test_fft_2d.hpp
    :language: cpp
    :lines: 17-

.. |image1| image:: ./media/image1.png
.. |image2| image:: ./media/image2.png
.. |image3| image:: ./media/image4.png
.. |image4| image:: ./media/image2.png
.. |image6| image:: ./media/image2.png
.. |image7| image:: ./media/image5.png
.. |image8| image:: ./media/image6.png
.. |image9| image:: ./media/image7.png
.. |image10| image:: ./media/image2.png
.. |image11| image:: ./media/image2.png
.. |image12| image:: ./media/image2.png
.. |image13| image:: ./media/image2.png
.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
