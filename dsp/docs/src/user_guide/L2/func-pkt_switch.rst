..
   Copyright (C) 2025, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _PKT_SWITCH:

===================
Packet Switch Graph
===================

The DSPLib contains a Packet Switch Graph.

Packet Switch Graph class wraps a DSP IP and adds a packet-switching streaming interface to the design.

.. _PKT_SWITCH_ENTRY:

Entry Point
===========

Packet Switch Graph has been placed in a distinct namespace scope: ``xf::dsp::aie``.

The graph entry point is as follows:

.. code-block::

    xf::dsp::aie::pkt_switch_graph

Device Support
==============

The class supports AIE, AIE-ML, and AIE-MLv2 devices.

Supported IP
===============

The Packet Switch Graph has been designed to support any IP that DSPLib offers, provided the IP is configured in a way that allows the addition of packet switching; see Design Notes for details in :ref:`PKT_SWITCH_CONFIGURATION`.


Template Parameters
===================

To see details on the template parameters for the Packet Switch Graph, see :ref:`API_REFERENCE`.

Access Functions
================

For the access functions for the Packet Switch Graph, see :ref:`API_REFERENCE`.

Ports
=====

To see the ports for the Packet Switch Graph, see :ref:`API_REFERENCE`.

Design Notes
============

.. _PKT_SWITCH_CONFIGURATION:

Packet Switch Graph configuration
----------------------------------


The Packet Switch Graph class implements a graph that splits incoming packet streams into multiple streams, processes them using a wrapped graph instance, and then merges the processed streams back
into packet outputs.

Class requires the following parameters:

- Number of super sample rate streams the wrapped graph instance uses, ``TP_SSR``.
- Number of input packet ports, ``TP_INPUT_PORTS``.
- Number of output packet ports, ``TP_OUTPUT_PORTS``.
- Type of the wrapped graph instance, ``TT_GRAPH_TYPE``.


.. _PKT_SWITCH_SSR:

Super Sample Rate Configuration
-------------------------------

Packet Switch Graph class must be configured with ``TP_SSR``.

This parameter will be used to create a network of connections from the array of input packet ports: ``std::array<port<input>, TP_INPUT_PORTS> pkt_in`` , to the input ports of the instantiated DSP IP.

Template parameter ``TP_SSR`` of the ``pkt_switch_graph`` class must match with the configuration of the instanced DSP IP.

.. _PKT_SWITCH_INPUTS:

Input Packet Ports
------------------

Packet Switch Graph class must be configured with ``TP_INPUT_PORTS``.

This parameter will be used to create an array of input ports: ``std::array<port<input>, TP_INPUT_PORTS> pkt_in`` , each capable of receiving a separate set of packet streams.
Each input port can support up to 32 different packet streams. See `UG1079 Explicit Packet Switching <https://docs.amd.com/r/en-US/ug1079-ai-engine-kernel-coding/Explicit-Packet-Switching>`_ for more details.

.. _PKT_SWITCH_INPUT_BUFFER_CONFIG:

Input Buffer configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^

Packet Switch Graph class is not aware of buffer configuration the underlying design may use. Buffer size, margin, single buffer constraint are the parameters of the IP and are handled internally within the IP configuration.

.. _PKT_SWITCH_SPLITS:

Input Packet Split Connections
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Template parameter ``TP_INPUT_PORTS`` will also determine the number of the ``pktsplit`` graph constructs used by the ``pkt_switch_graph`` class.

The ``pktsplit`` takes an input packet stream and broadcasts the stream to a number of input stream ports used by the instantiated DSP IP.
See `UG1079 Packet Split and Merge Connection <https://docs.amd.com/r/en-US/ug1079-ai-engine-kernel-coding/Packet-Split-and-Merge-Connections>`_ for more details.

Number of streams each ``pktsplit`` element produces is determined by the below formula:

.. code-block::

   N_STREAMS_SPLIT_FROM_PKT = TP_SSR / TP_INPUT_PORTS


.. _PKT_SWITCH_OUTPUTS:

Output Packet Ports
-------------------

Packet Switch Graph class must be configured with ``TP_OUTPUT_PORTS``.

This parameter will be used to create an array of output ports: ``std::array<port<output>, TP_OUTPUT_PORTS> pkt_out`` , each capable of producing a separate set of packet streams.
Each output port can produce up to 32 different packet streams. See `UG1079 Explicit Packet Switching <https://docs.amd.com/r/en-US/ug1079-ai-engine-kernel-coding/Explicit-Packet-Switching>`_ for more details.

.. _PKT_SWITCH_OUTPUT_BUFFER_CONFIG:

Output Buffer configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Packet Switch Graph class is not aware of buffer configuration the underlying design may use. Buffer size, single buffer constraint are the parameters of the IP and are handled internally within the IP configuration.

.. _PKT_SWITCH_MERGERS:

Output Packet Merge Connections
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Template parameter ``TP_OUTPUT_PORTS`` will also determine the number of the ``pktmerge`` graph constructs used by the ``pkt_switch_graph`` class.

The ``pktmerge`` takes a number of input packet streams and merges them into a single output packet stream.
See `UG1079 Packet Split and Merge Connections <https://docs.amd.com/r/en-US/ug1079-ai-engine-kernel-coding/Packet-Split-and-Merge-Connections>`_ for more details.

Number of streams each ``pktmerge`` element merges is determined by the below formula:

.. code-block::

   N_STREAMS_MERGES_TO_PKT = TP_SSR / TP_OUTPUT_PORTS


.. _PKT_SWITCH_LIMITS:

Limitations
-----------

The Packet Switch Graph class has been designed to work with any DSP IP that meets the following criteria:
- IP can be configured with single input port array.
- IP can be configured with single output port array
- IP can be configured with SSR operation,
- IP supports none or single graph constructor argument, e.g. taps array.

Constraints
-----------

The Packet Switch Graph class has been designed to create an instance of the desired DSP IP as a class member, i.e.:

.. code-block::

    TT_GRAPH_TYPE graph_instance;

Therefore, all constraints of the DSP IP can be passed through the pkt_switch_graph class by accessing the ``graph_instance`` member, or through a set of access functions, e.g.:

- `getKernels()` which returns a pointer to an array of kernel pointers.

More details are provided in the :ref:`API_REFERENCE`.



.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
