..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2023, Advanced Micro Devices, Inc.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.



.. _Mixed_Radix_FFT:

==========
FFT Window
==========

This library element implements an FFT or Inverse FFT of a point size which is not a power of 2, but is a product of a power of 2, a power of 3 and a power of 5.
Table :ref:`Mixed_Radix_FFT_HEADER_FORMAT` lists the template parameters used to configure the top level graph of the mixed_radix_fft_graph class.

~~~~~~~~~~~
Entry Point
~~~~~~~~~~~

The graph entry point is the following:

.. code-block::

    xf::dsp::aie::fft::mixed_radix_fft_graph

~~~~~~~~~~~~~~~
Supported Types
~~~~~~~~~~~~~~~

The data type to the FFT window is controlled by the template parameter TT_DATA. This may take one of 3 choices: cint16 or  cint32. TT_DATA determined the data type of  both input data and output data.
The template parameter TT_TWIDDLE is constrained by TT_DATA and so currently must be set to cint16.

~~~~~~~~~~~~~~~~~~~
Template Parameters
~~~~~~~~~~~~~~~~~~~

To see details on the template parameters for the Mixed Radix FFT, see :ref:`API_REFERENCE`.


~~~~~~~~~~~~~~~~
Access functions
~~~~~~~~~~~~~~~~

To see details on the access functions for the Mixed Radix FFT, see :ref:`API_REFERENCE`.

~~~~~
Ports
~~~~~

To see details on the ports for the Mixed Radix FFT, see :ref:`API_REFERENCE`. Note that the number and type of ports are determined by the configuration of template parameters.

~~~~~~~~~~~~
Design Notes
~~~~~~~~~~~~

The Mixed Radix FFT performs an FFT or inverse FFT on a frame of data of point size N where N = 2^A * 3^B * 5^C. The minimum value of A is 4 which means all supported point sizes are a multiple of 16. The maximum point size is determined by the amount of memory available to an AIE tile, so will be in the region of 2048. 

Dynamic Point Size
------------------

The Mixed Radix FFT does not currently support dynamic (run-time controlled) point sizes. 

Super Sample Rate Operation
---------------------------

Suport Sample Rate is commonly understood as a sample rate greater than the system clock. In the context of AIE kernels which are inherently SSR, the term SSR refers to execution using multiple kernels in parallel. The Mixed Radix FFT does not currently support implementations using multiple kernels in parallel to execute the FFT.


Super Sample Rate Sample to Port Mapping
////////////////////////////////////////

When Super Sample Rate operation is used, data is input and output using multiple ports. These multiple ports on input or output act as one channel. The mapping of samples to ports is that each successive sample should be passed to a different port in a round-robin fashion, e.g. with TP_SSR set to 4, samples 0, 4, 8, ... should be sent to input port 0, samples 1, 5, 9, ... to input port 1, samples 2, 6, 10, ... to input port 2, sample 3, 7, 11, ... to input port 3 and so on.

Scaling
-------
Scaling in the Mixed Radix FFT is controlled by the TP_SHIFT parameter which describes how many binary places by which to shift the result to the right, i.e. only power-of-2 scaling values are supported. Scaling is applied after the last rank has been computed. Internal calculations are unscaled, so a data type of cint32 is used internally to allow for bit-growth from an input of cint16. No larger type than cint32 is available, so if TT_DATA is cint32, the user must avoid arithmetic overflow by pre-scaling if necessary.

Rounding and Saturation
----------
In the final stage, the final values are converted to TT_DATA using TP_SHIFT, TP_RND and TP_SAT. TP_SHIFT performs the scaling as described previously. TP_RND and TP_SAT determine the form of rounding and saturation applied on the downshifted value. The following tables describes the form of rounding and of saturation performed.

.. _mixed_radix_fft_rnd_and_sat:

.. table:: Rounding and Saturation in Mixed Radix FFT
   :align: center

   +------------------------+----------------+-----------------+---------------------------------+
   | Parameter Name         |    Type        |  Description    |    Range                        |
   +========================+================+=================+=================================+
   |    TP_RND              |    Unsigned    | Round mode      |    0 = ``rnd_floor``            |
   |                        |    int         |                 |    truncate or                  |
   |                        |                |                 |    floor                        |
   |                        |                |                 |                                 |
   |                        |                |                 |    1 = ``rnd_ceil``             |
   |                        |                |                 |    ceiling                      |
   |                        |                |                 |                                 |
   |                        |                |                 |    2 = ``rnd_pos_inf``          |
   |                        |                |                 |    positive                     |
   |                        |                |                 |    infinity                     |
   |                        |                |                 |                                 |
   |                        |                |                 |    3 = ``rnd_neg_inf``          |
   |                        |                |                 |    negative                     |
   |                        |                |                 |    infinity                     |
   |                        |                |                 |                                 |
   |                        |                |                 |    4 = ``rnd_sym_inf``          |
   |                        |                |                 |    symmetrical                  |
   |                        |                |                 |    to infinity                  |
   |                        |                |                 |                                 |
   |                        |                |                 |    5 = ``rnd_sym_zero``         |
   |                        |                |                 |    symmetrical                  |
   |                        |                |                 |    to zero                      |
   |                        |                |                 |                                 |
   |                        |                |                 |    6 = ``rnd_conv_even``        |
   |                        |                |                 |    convergent                   |
   |                        |                |                 |    to even                      |
   |                        |                |                 |                                 |
   |                        |                |                 |    7 = ``rnd_conv_odd``         |
   |                        |                |                 |    convergent                   |
   |                        |                |                 |    to odd                       |
   +------------------------+----------------+-----------------+---------------------------------+
   |    TP_SAT              |    Unsigned    | Saturation mode |    0 = ``unsaturated``          |
   |                        |    int         |                 |                                 |
   |                        |                |                 |    1 = ``asymmetric saturation``|
   |                        |                |                 | i.e +2^(N-1)-1 to -2^(N-1)      |
   |                        |                |                 | e.g. +32767 to -32768           |
   |                        |                |                 |                                 |
   |                        |                |                 |    3 = ``symmetric saturation`` |
   |                        |                |                 | i.e +2^(N-1)-1 to -2^(N-1)+1    |
   |                        |                |                 | e.g. +32767 to -32767           |
   |                        |                |                 |                                 |
   +------------------------+----------------+-----------------+---------------------------------+


Constraints
-----------

The Mixed Radix FFT does not contain any tool constraints such as location constraints. 

Code Example
------------
.. literalinclude:: ../../../../L2/examples/docs_examples/test_mixed_radix_fft.hpp
    :language: cpp
    :lines: 15-

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



