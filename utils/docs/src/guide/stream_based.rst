.. 
     Copyright 2019-2023 Advanced Micro Devices, Inc
  
`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_

.. _stream_based:

***************************************************
Stream-Based API Design
***************************************************

.. toctree::
   :maxdepth: 1

Stream-based Interface
======================

The interfaces of the primitives in this library are mostly HLS streams, with a 1-bit flag stream
along with the main data stream throughout the dataflow region.

.. code:: cpp

        hls::stream<ap_uint<W> >& data_strm,
        hls::stream<bool>&        e_data_strm,

The packet data protocol in stream-based design is illustrated in the following figure.
For each valid data present in ``data_strm``, a ``false`` value presents the corresponding
``e_data_strm``. Meanwhile, an appended ``true`` value has to be given to close this packet.
So, stream consumer can be notified when data transfer is over. For a given packet,
the number of elements in ``e_data_strm`` is always one more than in corresponding
``data_strm`` during each transaction.

.. image:: /images/stream_based_protocol.png
   :alt: The protocl of stream packet data
   :scale: 80%
   :align: center

The benefits of this interface are as follows:

* Within a HLS dataflow region, all primitives connected via HLS streams can work in
  parallel, and this is the key to FPGA acceleration.

* Using the 1-bit flag stream to mark *end of operation* might trigger stream consumer
  as soon as the first row data becomes available, without knowing how many rows are
  generated later. Moreover, it might represent an empty table.


