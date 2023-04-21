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


.. _FILTERS_AIEML:

=================
Filters on AIE-ML
=================

The DSPLib contains several variants of Finite Impulse Response (FIR) filters.
These include single-rate FIRs, half-band interpolation/decimation FIRs, as well as integer and fractional interpolation/decimation FIRs. More details in the :ref:`FILTER_ENTRY`.



.. _FILTER_ENTRY_AIEML:

~~~~~~~~~~~
Entry Point
~~~~~~~~~~~

FIR filters have been categorized into classes and placed in a distinct namespace scope: ``xf::dsp::aie::fir``, to prevent name collision in the global scope. Namespace aliasing can be utilized to shorten instantiations:

.. code-block::

    namespace dsplib = xf::dsp::aie;

Additionally, each FIR filter has been placed in its unique FIR type namespace. The available FIR filter classes and the corresponding graph entry point are listed below:

.. _tab-fir-filter-classes-aieml:

.. table:: FIR Filter Classes
   :align: center

   +----------------------------------+-----------------------------------------------------------+
   |    **Function**                  | **Namespace and class name**                              |
   +==================================+===========================================================+
   | Single rate, asymmetrical        | dsplib::fir::sr_asym::fir_sr_asym_graph                   |
   +----------------------------------+-----------------------------------------------------------+
   | Single rate, symmetrical         | dsplib::fir::sr_sym::fir_sr_sym_graph                     |
   +----------------------------------+-----------------------------------------------------------+


~~~~~~~~~~~~~~~
Supported Types
~~~~~~~~~~~~~~~
FIRs can be configured for various types of data and coefficients. These types can be int16, and real or complex.
Certain combinations of data and coefficient type are not supported.

The following table lists the supported combinations of data type and coefficient type with notes for those combinations not supported.

.. _tab_supported_combos_aieml:

.. table:: Supported Combinations of Data Type and Coefficient Type
   :align: center

   +-------------------------------+------------------------------------+
   |                               |     **Data        Type**           |
   |                               +------------------+-----------------+
   |                               | **Int16**        | **Cint16**      |
   +----------------------+--------+------------------+-----------------+
   | **Coefficient type** | Int16  | Supported        | Supported       |
   |                      +--------+------------------+-----------------+
   |                      | Cint16 | note 1           | Supported       |
   +----------------------+--------+------------------+-----------------+
   | 1. Complex coefficients are not supported for real-only data types.|
   +--------------------------------------------------------------------+


~~~~~~~~~~~~~~~~~~~
Template Parameters
~~~~~~~~~~~~~~~~~~~

The following table lists parameters for the FIR filters on AIE-ML.

.. note:: ``TP_RND`` values may vary between AIE and AIE-ML for the same rounding mechanism. String macro defitions are recommended for convienience and compatibility between devices. For AIE, see :ref:`fir_supported_params`.

.. _fir_supported_params_aieml:

.. table:: Parameters Supported by FIR Filters
   :align: center

   +------------------------+----------------+-----------------+---------------------------------+
   | Parameter Name         |    Type        |  Description    |    Range                        |
   +========================+================+=================+=================================+
   |    TT_DATA             |    Typename    | Data Type       |    int16,                       |
   |                        |                |                 |    cint16                       |
   +------------------------+----------------+-----------------+---------------------------------+
   |    TT_COEFF            |    Typename    | Coefficient     |    int16                        |
   |                        |                | type            |    cint16                       |
   +------------------------+----------------+-----------------+---------------------------------+
   |    TP_FIR_LEN          |    Unsigned    | The number of   | Min - 4,                        |
   |                        |    int         | taps            |                                 |
   |                        |                |                 | Max - see :ref:`MAX_FIR_LENGTH` |
   +------------------------+----------------+-----------------+---------------------------------+
   |    TP_SHIFT            |    Unsigned    | The number of   |    0 to 61                      |
   |                        |    int         | bits to shift   |                                 |
   |                        |                | unscaled        |                                 |
   |                        |                | result          |                                 |
   |                        |                | down by before  |                                 |
   |                        |                | output.         |                                 |
   +------------------------+----------------+-----------------+---------------------------------+
   |    TP_RND              |    Unsigned    | Round mode      |    0 = ``rnd_floor``            |
   |                        |    int         |                 |    truncate or                  |
   |                        |                |                 |    floor                        |
   |                        |                |                 |                                 |
   |                        |                |                 |    1 =  ``rnd_ceil``            |
   |                        |                |                 |    ceiling                      |
   |                        |                |                 |                                 |
   |                        |                |                 |    2 = ``rnd_sym_floor``        |
   |                        |                |                 |    floor to zero                |
   |                        |                |                 |                                 |
   |                        |                |                 |    3 = ``rnd_sym_ceil``         |
   |                        |                |                 |    ceil to                      |
   |                        |                |                 |    infinity                     |
   |                        |                |                 |                                 |
   |                        |                |                 |    8 = ``rnd_neg_inf``          |
   |                        |                |                 |    round to negative            |
   |                        |                |                 |    infinity                     |
   |                        |                |                 |                                 |
   |                        |                |                 |    9 = ``rnd_pos_inf``          |
   |                        |                |                 |    round to positive            |
   |                        |                |                 |    infinity                     |
   |                        |                |                 |                                 |
   |                        |                |                 |    10 = ``rnd_sym_zero``        |
   |                        |                |                 |    symmetrical                  |
   |                        |                |                 |    to zero                      |
   |                        |                |                 |                                 |
   |                        |                |                 |    11 = ``rnd_sym_inf``         |
   |                        |                |                 |    symmetrical                  |
   |                        |                |                 |    to infinity                  |
   |                        |                |                 |                                 |
   |                        |                |                 |    12 = ``rnd_conv_even``       |
   |                        |                |                 |    convergent                   |
   |                        |                |                 |    to even                      |
   |                        |                |                 |                                 |
   |                        |                |                 |    13 = ``rnd_conv_odd``        |
   |                        |                |                 |    convergent                   |
   |                        |                |                 |    to odd                       |
   +------------------------+----------------+-----------------+---------------------------------+
   | TP_INPUT_WINDOW_VSIZE  |    Unsigned    | The number      |  Must be a                      |
   |                        |    int         | of samples      |  multiple of                    |
   |                        |                | processed by    |  the 256-bits                   |
   |                        |                | the graph in a  |  In addition, must by           |
   |                        |                | single          |  divisible by:                  |
   |                        |                | iteration run.  |  ``TP_SSR`` and                 |
   |                        |                | For windows,    |  ``TP_DECIMATE_FACTOR``.        |
   |                        |                | defines the     |                                 |
   |                        |                | size of input   |  No                             |
   |                        |                | window. For     |  enforced                       |
   |                        |                | streams, it     |  range, but                     |
   |                        |                | impacts the     |  large                          |
   |                        |                | number of input |  windows                        |
   |                        |                | samples operated|  will result                    |
   |                        |                | on in a single  |  in mapper                      |
   |                        |                | iteration       |  errors due                     |
   |                        |                | of the kernel.  |  to                             |
   |                        |                |                 |  excessive                      |
   |                        |                |                 |  RAM use, for windowed          |
   |                        |                |                 |  API implementations.           |
   |                        |                |                 |                                 |
   +------------------------+----------------+-----------------+---------------------------------+
   |    TP_CASC_LEN         |    Unsigned    | The number      |    1 to 9.                      |
   |                        |    int         | of cascaded     |                                 |
   |                        |                | kernels to      |    Defaults to                  |
   |                        |                | use for         |    1 if not                     |
   |                        |                | this FIR.       |    set.                         |
   |                        |                |                 |                                 |
   +------------------------+----------------+-----------------+---------------------------------+
   | TP_USE_COEFF_RELOAD    |    Unsigned    | Enable          |    0 (no                        |
   |                        |    int         | reloadable      |    reload), 1                   |
   |                        |                | coefficient     |    (use                         |
   |                        |                | feature.        |    reloads).                    |
   |                        |                |                 |                                 |
   |                        |                | An additional   |    Defaults to                  |
   |                        |                | 'coeff'         |    0 if not                     |
   |                        |                | port will       |    set.                         |
   |                        |                | appear on       |                                 |
   |                        |                | the graph.      |                                 |
   +------------------------+----------------+-----------------+---------------------------------+
   |  TP_API                |    Unsigned    | I/O interface   |  0 = Window                     |
   |                        |    int         | port type       |                                 |
   |                        |                |                 |  1 = Stream                     |
   +------------------------+----------------+-----------------+---------------------------------+


For a list of template parameters for each FIR variant, see :ref:`API_REFERENCE`.

**TP_CASC_LEN** describes the number of AIE processors to split the operation over, which allows resources to be traded for higher performance. ``TP_CASC_LEN`` must be in the range 1 (default) to 9.
FIR graph instance creates ``TP_CASC_LEN`` kernels. Computation workload of the FIR (defined by its length parameter ``TP_FIR_LEN``) is divided and each kernel in the graph is then assigned a fraction of the workload, i.e. each kernel performs ``TP_FIR_LEN / TP_CASC_LEN``.
Kernels are connected with cascade ports, which pass partial accumulation products downstream until last kernel in chain produces the output.

**TP_USE_COEFF_RELOAD**  allows the user to select if runtime coefficient reloading should be used.
When defining the parameter:

* 0 = static coefficients, defined in filter constructor

* 1 = reloadable coefficients, passed as argument to runtime function.

  .. note:: When used, port ``port<input> coeff;`` will be added to the FIR.

~~~~~~~~~~~~~~~~
Access functions
~~~~~~~~~~~~~~~~

For the access functions for each FIR variant, see :ref:`API_REFERENCE`.

~~~~~
Ports
~~~~~

To see the ports for each FIR variants, see :ref:`API_REFERENCE`. Note that some ports are present only for certain configurations of template parameters.

~~~~~~~~~~~~
Design Notes
~~~~~~~~~~~~

Coefficient array for Filters
-------------------------------

Static coefficients
///////////////////

For all non-reloadable filter configurations, the coefficient values are passed as an array argument to the constructor as either std::array or std::vector.

Static Coefficients - array size
////////////////////////////////

**Asymmetrical FIR**

Asymmetrical filters expect the port to contain the full array of coefficients, i.e. coefficient array size is equal to the ``TP_FIR_LEN``.

**Symmetrical FIR**

| In the case of symmetrical filters, only the first half (plus any odd centre tap) need be passed, as the remaining may be derived by symmetry.
| The length of the array expected will therefore be ``(TP_FIR_LEN+1)/2``, e.g. for a filter of length 7, where coeffs are ``int16``:
| ``{1, 2, 3, 5, 3, 2, 1}``, 4 non-zero tap values, including the centre tap, are expected, i.e. constructor expects an argument:
| ``std::array<int16, 4> tapsIn =  {1, 2, 3, 5}``.


Reloadable coefficients
///////////////////////

Reloadable coefficients are available through the use of run-time programmable (RTP) Asynchronous input port, programmed by Processor Subsystem (PS) at run-time.
Reloadable configurations do not require the coefficient array to be passed to the constructor at compile time.
Instead, the graph's `update()` (refer to `UG1079 Run-Time Parameter Update/Read Mechanisms <https://docs.xilinx.com/r/en-US/ug1079-ai-engine-kernel-coding/Run-Time-Parameter-Update/Read-Mechanisms>`_ for usage instructions) method is used to input the coefficient array.
Graph's `update()` method takes an argument of either scalar or an array type.
Please refer to `UG1079 Run-Time Parameter Support Summary <https://docs.xilinx.com/r/en-US/ug1079-ai-engine-kernel-coding/Run-Time-Parameter-Support-Summary>`_.

.. note:: Graph's `update()` method must be called after graph has been initialized, but before kernel starts operation on data samples.


Reloadable Coefficients - array dimensions
//////////////////////////////////////////

FIR filters expect the port to contain the full array of coefficients, i.e. coefficient array size is equal to the ``TP_FIR_LEN``.

| In the case of symmetrical filters, the size of each port will be dependent on the underlying kernel structure Asymmetric FIR.
| As a result, deriving symmetric coefficients from the argument passed to graph's `update()` method is not available.
| The length of the array expected will therefore be ``(TP_FIR_LEN)``,
| e.g. for a filter of length 7, where ``int16`` type coefficient's values are:
| ``{1, 2, 3, 5, 3, 2, 1}``,
| 7 non-zero tap values, including the centre tap, are expected, i.e. `update()` method should get an argument:
| ``int16 tapsIn[7] =  {1, 2, 3, 5, 3, 2, 1}``.

| A helper function: ``convert_sym_taps_to_asym`` is provided in the `Graph utils <../../rst/group_graph_utils.html>`_ to ease converting taps array to the required format.

Window interface for Filters
----------------------------

See :ref:`WINDOW_API_FIRS`.



Streaming interface for Filters
-------------------------------

Streaming interfaces are based on 32-bit AXI4-Stream and offer throughput of up to 32 Gbps (based on 1 GHz AIE) per stream used.

When ``TP_API = 1`` the FIR will have stream API input and output ports, allowing greater interoperability and flexibility in placement of the design.

Single Rate FIRs will use input and output streams directly.
As a result, there is no need for input/output buffering, hence streaming FIRs offer very low latency and very low memory footprint.
In addition, due to the lack of memory requirements, such designs may operate on very large number of samples within each kernel iteration ``TP_INPUT_WINDOW_VSIZE`` is limited to ``2^31 - 1``  achieving maximum performance and maximum throughput.

For example, a single kernel (``TP_CASC_LEN = 1``), 16 tap single-rate asymmetric FIR, using ``cint16`` data with frame size of `25600` and ``int16`` coefficients, is offering throughput of `998 MSa/s` (based on 1 GHz AIE clock) and latency as low as tens of nanoseconds.

.. _FIR_STREAM_OUTPUT_AIEML:

Stream Output
/////////////

Stream output allows computed data samples to be sent directly over the stream without the requirement for a ping-pong window buffer.
As a result, memory use and latency are reduced.
Furthermore, the streaming output allows data samples to be broadcast to multiple destinations.

Maximum FIR Length
------------------

See :ref:`MAX_FIR_LENGTH`.

Minimum Cascade Length
----------------------

See :ref:`MINIUM_CASC_LEN`.

Optimum Cascade Length
----------------------
See :ref:`OPTIMUM_CASC_LEN`.


.. _FIR_CONSTRAINTS_AIEML:

Constraints
-----------

Each FIR variant has a variety of access methods to help assign a constraint on a kernel and/or a net, e.g.:

- `getKernels()` which returns a pointer to an array of kernel pointers, or

- `getInNet()` which returns a pointer to a net indexed by method's argument(s).

More details are provided in the  :ref:`API_REFERENCE`.

An example of how to use this is given in the section :ref:`FIR_CODE_EXAMPLE`.

.. code-block::

   Kernel Index = Kernel Cascade index

The nets returned by the `getInNet()` function can be assigned custom fifo_depths values to override the defaults.

FIR Code Example
----------------
See :ref:`FIR_CODE_EXAMPLE`.




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



