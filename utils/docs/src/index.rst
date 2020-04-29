.. 
   Copyright 2019 Xilinx, Inc.
  
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
  
       http://www.apache.org/licenses/LICENSE-2.0
  
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

=====================
Vitis Utility Library
=====================

Vitis Utility Library is an open-sourced Vitis library of common patterns of streaming and storage access.
It aims to assist developers to efficiently access memory in DDR, HBM or URAM, and perform data distribution,
collection, reordering, insertion, and discarding along stream-based transfer.

**Memory Access**

* **AXI Burst Read and Write**:
  Reading data from AXI master port, and emit to stream (of possibly different width).
  Padding and vectoring could be removed in reading and added in writing.

* **Low Initiation Interval Access to URAM Array**:
  URAMs are 72 bit fixed, and currently very large buffers needs extra registers and forwarding-paths to be read/written every cycle.
  By providing an API for this, users can focus on the function design and avoid mixing challenge of different level in the same code.

**Dynamic Routing within Streams**

* **Distribution and Collection**:
  Three different algorithms are supported:

  * **Round-robin**: Data will be send/received through as many streams as possible in round-robin order.
  * **Load-balancing**: Data will be send to or received from PU based on load, the result will be out of order. The assumption here is a PU has lower rate than the input, while the cluster of PUs has statistically matching rate with the input.
  * **Tag-select**: Basically it implements the ``if-then-else`` or ``switch-case`` semantic, using a tag from input. This module can be used to pass different type/category of data to heterogeneous PUs.

* **Discarding Data**:
  Consumes all the data from input and discard it. With some post-bitstream configuration, we may have some data generated but not used,
  this module ensures such data can be discarded without blocking the execution.

* **Stream Synchronization**:
  We have made end flag driven pipeline a common practice in our libraries,
  and it is often necessary to synchronize two streams into one flag signal.

**Data Reshaping**

* **Stream Combine**:
  Fuse multiple streams into one. For example, fuse three synchronized R, G, B streams into one (R,G,B) tuple stream.
  The dynamic version allows some streams to be skipped(discarded) while allowing two static directions (align to LSB or MSB).

* **Stream Shuffle**:
  Shuffle synchronized streams with dynamic configuration.
  For example, synchronized R, G, B streams can be dynamically shuffle to R, B, G or B, G, R.


.. toctree::
   :caption: Library Overview
   :maxdepth: 1

   overview.rst
   release.rst

.. toctree::
   :caption: User Guide
   :maxdepth: 3

   guide/toc.rst

Index
-----

* :ref:`genindex`

Library API Summary
-------------------

+------------------+-------------------------------------------------------------------------------------------+-------+
| Library Function | Description                                                                               | Layer |
+==================+===========================================================================================+=======+
| axiToMultiStream | Loading multiple categories of data from one AXI master to streams                        | L1    |
+------------------+-------------------------------------------------------------------------------------------+-------+
| axiToStream      | Loading data elements from AXI master to stream                                           | L1    |
+------------------+-------------------------------------------------------------------------------------------+-------+
| axiToCharStream  | Loading char data from AXI master to stream                                               | L1    |
+------------------+-------------------------------------------------------------------------------------------+-------+
| streamCombine    | combine multiple streams into a wide one, shift selected streams to LSB or MSB side       | L1    |
+------------------+-------------------------------------------------------------------------------------------+-------+
| streamDiscard    | Discard multiple streams                                                                  | L1    |
+------------------+-------------------------------------------------------------------------------------------+-------+
| streamDup        | Duplicate stream                                                                          | L1    |
+------------------+-------------------------------------------------------------------------------------------+-------+
| streamNToOne     | A group of overloaded functions, providing ability to collect data from multiple streams  | L1    |
|                  | using select algorithm from round-robin, load-balancing and tag-select.                   |       |
+------------------+-------------------------------------------------------------------------------------------+-------+
| streamOneToN     | A group of overloaded functions, providing ability to distribute data to multiple streams | L1    |
|                  | using select algorithm from round-robin, load-balancing and tag-select.                   |       |
+------------------+-------------------------------------------------------------------------------------------+-------+
| streamReorder    | Window-reorder in a stream                                                                | L1    |
+------------------+-------------------------------------------------------------------------------------------+-------+
| streamShuffle    | Shuffle the contents from an array of streams to another                                  | L1    |
+------------------+-------------------------------------------------------------------------------------------+-------+
| streamSplit      | split one wide stream into multiple streams                                               | L1    |
+------------------+-------------------------------------------------------------------------------------------+-------+
| streamSync       | Synchronize streams for successor module                                                  | L1    |
+------------------+-------------------------------------------------------------------------------------------+-------+
| streamToAxi      | Write elements in burst to AXI master port                                                | L1    |
+------------------+-------------------------------------------------------------------------------------------+-------+

+---------------+----------------------------------------------------------------------------------------+-------+
| Library Class | Description                                                                            | Layer |
+===============+========================================================================================+=======+
| UramArray     | Helper class to create URAM array that can be updated every cycle with forwarding regs | L1    |
+---------------+----------------------------------------------------------------------------------------+-------+

