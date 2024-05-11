..
   Copyright © 2019–2024 Advanced Micro Devices, Inc
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _FIR_TDM:

=======
TDM FIR
=======

The DSPLib contains a Time-Division Multiplexing (TDM) variant of finite impulse response (FIR) filter.
It is a multi-channel FIR filter with configurable application parameters number of channels, FIR length, etc. as well as implementation parameters , e.g. IO buffer size or super sample rate (SSR) operation mode.
.. _FILTER_ENTRY:

Entry Point
===========

TDM FIR have been placed in a distinct namespace scope: ``xf::dsp::aie::fir::tdm``.

The graph entry point is the following:

.. code-block::

    xf::dsp::aie::fir::tdm::fir_tdm_graph

Device Support
==============

The TDM FIR filter supports both AIE1 and AIE-ML for all features with the following exceptions:

- TP_DUAL_IP == 1 is supported on AIE1, but not on AIE-ML
- TP_NUM_OUTPUTS == 2 is supported on AIE1, but not on AIE-ML
- Round modes available and the enumerated values of round modes differ between AIE1 and AIE-ML. See :ref:`COMPILING_AND_SIMULATING`.

Supported Types
===============

TDM FIR filters can be configured for various types of data and coefficients. These types can be int16, int32, or float, and also real or complex. Certain combinations of data and coefficient type are not supported.

The following table lists the supported combinations of data type and coefficient type with notes for those combinations not supported.

.. _tab_supported_combos:

.. table:: Supported Combinations of Data Type and Coefficient Type
   :align: center

   +-----------------------------------+--------------------------------------------------------------------------+
   |                                   |                                 **Data Type**                            |
   |                                   +-----------+------------+-----------+------------+-----------+------------+
   |                                   | **Int16** | **Cint16** | **Int32** | **Cint32** | **Float** | **Cfloat** |
   |                                   |           |            |           |            |           | (note 3)   |
   +----------------------+------------+-----------+------------+-----------+------------+-----------+------------+
   | **Coefficient type** | **Int16**  | Supported | Supported  | Supported | Supported  | note 2    | note 2     |
   |                      |            |           |            |           |            |           |            |
   |                      +------------+-----------+------------+-----------+------------+-----------+------------+
   |                      | **Cint16** | note 1    | Supported  | note 1    | Supported  | note 2    | note 2     |
   |                      +------------+-----------+------------+-----------+------------+-----------+------------+
   |                      | **Int32**  | Supported | Supported  | Supported | Supported  | note 2    | note 2     |
   |                      |            |           |            |           |            |           |            |
   |                      +------------+-----------+------------+-----------+------------+-----------+------------+
   |                      | **Cint32** | note 1    | Supported  | note 1    | Supported  | note 2    | note 2     |
   |                      +------------+-----------+------------+-----------+------------+-----------+------------+
   |                      | **Float**  | note 2    | note 2     | note 2    | note 2     | Supported | Supported  |
   |                      +------------+-----------+------------+-----------+------------+-----------+------------+
   |                      | **Cfloat** | note 2    | note 2     | note 2    | note 2     | note 1    | Supported  |
   |                      | (note 3)   |           |            |           |            |           |            |
   +----------------------+------------+-----------+------------+-----------+------------+-----------+------------+
   | 1. Complex coefficients are not supported for real-only data types.                                          |
   | 2. A mix of float and integer types is not supported.                                                        |
   | 3. The cfloat data type is not supported on AIE-ML device.                                                   |
   +--------------------------------------------------------------------------------------------------------------+

Template Parameters
===================

To see details on the template parameters for the TDM FIR, see :ref:`API_REFERENCE`.

Access Functions
================

For the access functions for each FIR variant, see :ref:`API_REFERENCE`.

Ports
=====

To see the ports for each FIR variants, see :ref:`API_REFERENCE`.

Design Notes
============

.. _COEFFS_FOR_FIRS:

Coefficient Array for Filters
-----------------------------

The coefficient array values are passed as an array argument to the constructor as either a single dimension ``std::array`` or a ``std::vector``.

Coefficients - Array Size
^^^^^^^^^^^^^^^^^^^^^^^^^

TDM FIR Coefficient array size is equal to the length of the FIR multiplied by number of channels, i.e.:

``Coeff_Array_Size = TP_FIR_LEN * TP_TDM_CHANNELS``

Coefficients - Array Organization
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Coefficient vector must be created in a form that lists set of taps for each channel at a ti as an argument to the constructor in the following form:

.. code-block::

std::vector<TT_DATA> coeffVector = {
                              CN-1.0, CN-1.1, CN-1.2, CN-1.2, ..., CN-1.M-2, CN-1.M-1,
                              CN-2.0, CN-2.1, CN-2.2, CN-2.2, ..., CN-2.M-2, CN-2.M-1,
                              ...
                              C2.0, C2.1, C2.2, C2.2, ..., C2.M-2, C2.M-1,
                              C1.0, C1.1, C1.2, C1.2, ..., C1.M-2, C1.M-1,
                              CN0.0, C0.1, C0.2, C0.2, ..., C0.M-2, C0.M-1,
                              };

where:

- N - FIR Length, i.e. number of taps on each channel (``TP_FIR_LEN``),
- M - Number of TDM Channels  (``TP_TDM_CHANNELS``).

.. _BUFFER_API_FIRS:

IO Buffer Interface for Filters
-------------------------------

On the AI Engine processor, data can be packetized into IO buffers, which are mapped to the local memory.

IO buffers can be accessed with a 256-bit wide load/store operation, hence offering a throughput of up to 256 Gb/s (based on 1 GHz AIE clock).

IO buffers are implemented using a `ping-pong` mechanism, where the consumer kernel would read the `ping` portion of the buffer while the producer would fill the `pong` portion of the buffer that would be consumed in the next iteration.

In each iteration run, the kernel operates on a set number of samples from the input buffer, defined by the template parameter ``TP_INPUT_WINDOW_VSIZE``. To allow the kernel to safely operate on buffered data, a mechanism of lock acquires and releases is implemented.

Margin
^^^^^^

Input buffer of a TDM FIR may be extended by a margin so that the state of the filter at the end of the previous iteration can be restored.

Amount of margin required by TDM FIR can be calculated using formula:

``Margin_samples = (TP_FIR_LEN - 1) * TP_TDM_CHANNELS``

Internal Margin
^^^^^^^^^^^^^^^

For cases where margin data exceeds new arriving data, i.e. when
``Margin_samples > TP_INPUT_WINDOW_VSIZE``
margin will be implemented as a separate buffer, internal to the kernel that operates on it.

As a result, the input buffers will not be extended by margin data.

.. note:: Internal Margin handling is not supported on AIE-ML device.

Maximizing Throughput
^^^^^^^^^^^^^^^^^^^^^

Buffer synchronization requirements introduce a fixed overhead when a kernel is triggered.
Therefore, to maximize throughput, the input buffer size should be set to the maximum that the system will allow.

.. note:: To achieve maximum performance, the producer and consumer kernels should be placed in adjacent AIE tiles, so the window buffers can be accessed without a requirement for a MM2S/S2MM direct memory access (DMA) stream conversions.

Multiple Frames
^^^^^^^^^^^^^^^

To minimize kernel switching overheads and therefore, maximize performance, TDM FIR supports batching multiple frames into a single input buffer.
A frame can defined as a set of data samples, one data sample for each TDM channel, i.e. a frame is a set of ``TP_TDM_CHANNELS`` input samples.

Input buffer size can be set to an integer multiple of TDM Channels, e.g.:
``TP_INPUT_WINDOW_VSIZE = TP_TDM_CHANNELS * NUMBER_OF_BATCHES``

A TDM FIR configured to operate on multiple frames in a given kernel iteration will produce equal amount of output frames.

Latency
^^^^^^^

Latency of a buffer-based TDM FIR is predominantly due to the buffering in the input and output buffers. Other factors which affect latency are data and type and FIR length, though these tend to have a lesser effect.

To minimize the latency, the buffer size should be set to the minimum size that meets the required throughput.

.. _FIR_MULTIPLE_BUFFER_PORTS:

Multiple Buffer Ports
^^^^^^^^^^^^^^^^^^^^^

TDM FIR only supports a single input/output port at this time.

.. _MAX_WINDOW_SIZE:

Maximum Window Size
^^^^^^^^^^^^^^^^^^^

Window buffer is mapped into a local memory in the area surrounding the kernel that accesses it.

A local memory storage is 32 kB (64 kB for AIE-ML devices), and the maximum size of the `ping-pong` window buffer should not exceed this limit.

.. note:: Input buffers may be extended by margin data, which can significantly reduce the maximum window size.


.. _SINGLE_BUFFER_CONSTRAINT:

Single Buffer Constraint
^^^^^^^^^^^^^^^^^^^^^^^^

| It is possible to disable the `ping-pong` mechanism, so that the entire available data memory is available to the kernel for computation. However, the single-buffered window can be accessed only by one agent at a time, and it comes with a performance penalty.
| This can be achieved by using the `single_buffer()` constraint that is applied to an input or output port of each kernel.

.. code-block::

    single_buffer(firGraph.getKernels()[0].in[0]);

.. _STREAM_API_FIRS:

Streaming Interface for Filters
-------------------------------

Streaming interfaces are not supported by TDM FIR.

.. _SSR_OPERATION:

Super Sample Rate
-----------------

The term Super Sample Rate strictly means the processing of more than one sample per clock cycle. Because the AIE is a vector processor, almost every operation is SSR by this definition, making it superfluous. Therefore, in the AIE context, SSR is taken to mean an implementation using multiple computation paths to improve performance at the expense of additional resource use.

.. _SSR_OPERATION_MODES:

Super Sample Rate - Operation Modes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

TDM FIR can be configured to operate in SSR Mode using ``TP_SSR`` template parameter.  The mode will create an array of ``TP_SSR`` kernels and create the ``TP_SSR`` amount of the **input** and **output** ports.

.. _SSR_OPERATION_RESOURCE_UTILIZATION:

Super Sample Rate - Resource Utilization
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The number of AIE tiles used by a FIR will be given by the formula:

.. code-block::

  NUMBER_OF_AIE_TILES = TP_SSR


.. _SSR_OPERATION_PORT_UTILIZATION:

Super Sample Rate - Port Utilization and Throughput
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The number of input/output ports created by a FIR will be given by the formula:

* Number of input ports: ``NUM_INPUT_PORTS  = TP_SSR``

* Number of output ports: ``NUM_OUTPUT_PORTS  = TP_SSR``

Therefore, the maximum throughput achievable for a given data type, e.g., cint16 and 1 GHz AIE Clock, can be estimated with:

* maximum theoretical sample rate at input: ``THROUGHPUT_IN  = NUM_INPUT_PORTS x 1 GSa/s``,

* maximum theoretical sample rate at output = ``THROUGHPUT_OUT  = NUM_OUTPUT_PORTS x 1 GSa/s``.


**AIE Tile Utilization Ratio**

A Super Sample Rate operation creates multiple computation paths that are used to produce the output samples.
Having multiple computation paths reduces the amount of computation required by each kernel.

The total number of FIR computation paths can be described with the following formula:

.. code-block::

  NUMBER_OF_COMPUTATION_PATHS = TP_SSR

TDM FIR graph will split the requested FIR workload among the FIR kernels equally, which can mean that each kernel is tasked with a comparatively low computational effort.

In such a scenario, the bandwidth will be limited by the amount of ports, but the AIE tile utilization ratio (often defined as ratio of VMAC operations to cycles without VMAC operation) might be reduced.

For example, a 32 tap Single Rate FIR operating on a ``cint16`` data type and ``int16`` coefficients with ``TP_SSR`` set to 2 and a cascade length ``TP_CASC_LEN`` set to 2 will perform at the bandwidth close to `2 GSa/s` (2 output stream paths). Each of the kernels will be tasked with computing only eight coefficients. The design will use eight FIR kernels mapped to eight AIE tiles to achieve that.
However, a similarly configured FIR, a 32 tap Single Rate FIR operating on ``cint16`` data type and ``int16`` coefficients with ``TP_SSR`` set to 2, but without further cascade configuration (``TP_CASC_LEN`` set to 1) would also perform at the bandwidth close to `2 GSa/s` but only consume four kernels to achieve that.

.. _SSR_OPERATION_COEFF_DATA_DISTRO:

Super Sample Rate - Coefficient and Data Distribution
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The base mode of SSR is driven by the use of the ``TP_SSR``template parameter.
The ``TP_SSR`` parameter allows a trade of performance for resource use in the form of tiles used.

When used, a number of ``TP_SSR`` input phases and a number of ``TP_SSR`` output paths will be created.
An array of ``TP_SSR^2`` FIR sub-graphs will be created to connect the input phases and output paths.

Input data samples are distributed across the input phases in a round-robin, sample-by-sample mechanism where each input phase processes a fraction of the input samples, i.e.,``TP_INPUT_WINDOW_VSIZE / TP_SSR``. More details in: :ref:`SSR_PORT_MAPPING`.

Coefficients are distributed in such way that each output path consists of all the FIRs coefficients, but each FIR sub-graph in any given output path is only configured to operate on a fraction of the FIR length, i.e., operates on ``TP_FIR_LEN / TP_SSR`` number of coefficients.

As a result, each FIR sub-graph operates on a fraction of coefficients and a fraction of the data, giving an overall increased performance.

In addition, each FIR sub-graph can be further split into multiple FIR kernels with the use of a cascade interface, which is driven by the ``TP_CASC_LEN`` template parameter.

For example, a FIR with ``TP_SSR=4`` and ``TP_CASC_LEN=2`` will create a kernel structure presented as follows, in :ref:`FIGURE_FIR_SSR`.

.. _FIGURE_FIR_SSR:

.. figure:: ./media/SSR_FIR_6_5in.png


   **Internal structure of FIR with TP_SSR=4 and TP_CASC_LEN=2**


.. _SSR_PORT_MAPPING:

Super Sample Rate - Sample to Port Mapping
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

When a Super Sample Rate operation is used, data is input and output using multiple ports. These multiple ports on input or output, act as one channel.

The input data channel must be split over multiple ports where each successive input sample is sent to a different input port in a round-robin fashion, i.e., sample 0 goes to input port :code:`in[0]`, sample 1 to :code:`in[1]`, etc. up to N-1 where N=TP_SSR, then sample N goes to :code:`in[0]`, sample N+1 goes to :code:`in[1]` and so on. Output samples are output from the multiple output ports in the same fashion.

For example, if you have a data stream like :code:`int32 x = 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, ...`, then an SSR of 3 with dual input ports would look like:

.. code-block::

  in[0] = 0, 3, 6, 9, 24, 27, 30, 33, ...
  in[1] = 1, 4, 7, 10, 25, 28, 31, 34, ...
  in[2] = 2, 5, 8, 11, 26, 29, 32, 35, ...
  in2[0] = 12, 15, 18, 21, 36, 39, 42, 45, ...
  in2[1] = 13, 16, 19, 22, 37, 40, 43, 46, ...
  in2[2] = 14, 17, 20, 23, 38, 41, 44, 47, ...

The output data will be produced in a similar method.

.. _FIR_TDM_CONSTRAINTS:

Constraints
-----------

TDM FIR variant has a variety of access methods to help assign a constraint on a kernel and/or a net, e.g.:

- `getKernels()` which returns a pointer to an array of kernel pointers, or

- `getInNet()` which returns a pointer to a net indexed by method's argument(s).

More details are provided in the :ref:`API_REFERENCE`.

An example of how to use this is given in the section :ref:`FIR_TDM_CODE_EXAMPLE`.

.. _FIR_TDM_CODE_EXAMPLE:

Code Example
============

The following code example shows how a FIR graph class might be used within a user super-graph, including example code to set the runtime ratio of kernels within the FIR graph class.

.. literalinclude:: ../../../../L2/examples/docs_examples/test_fir_tdm.hpp
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
