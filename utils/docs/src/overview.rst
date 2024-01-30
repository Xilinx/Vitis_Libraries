.. 
   Copyright 2023 Advanced Micro Devices, Inc
  
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
  
       http://www.apache.org/licenses/LICENSE-2.0
  
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

.. _overview:

.. toctree::
   :hidden:

Overview
------------

AMD Vitis |trade| Utility Library is an open-sourced Vitis library of common patterns of streaming and storage access.
It aims to assist developers to efficiently access memory in DDR, HBM, or URAM, and perform data distribution,
collection, reordering, insertion, and discarding along stream-based transfer.

**Memory Access**

* **AXI Burst Read and Write**:
  Reading data from AXI master port, and emit to stream (of possibly different width).
  Padding and vectoring could be removed in reading and added in writing.

* **Low Initiation Interval Access to URAM Array**:
  URAMs are 72 bit fixed, and currently, large buffers need extra registers and forwarding-paths to be read/written every cycle.
  By providing an API for this, you can focus on the function design and avoid mixing challenge of different level in the same code.

**Dynamic Routing within Streams**

* **Distribution and Collection**:
  Three different algorithms are supported:

  * **Round-robin**: Data is sent/received through as many streams as possible in a round-robin order.
  * **Load-balancing**: Data is sent to or received from PU based on load, the result is out of order. The assumption here is that a PU has lower rate than the input, while the cluster of PUs has statistically matching rate with the input.
  * **Tag-select**: Basically, it implements the ``if-then-else`` or ``switch-case`` semantic, using a tag from input. This module can be used to pass different type/category of data to heterogeneous PUs.

* **Discarding Data**:
  Consumes all the data from input and discards it. With some post-bitstream configuration, we might have some data generated but not used,
  this module ensures that such data can be discarded without blocking the execution.

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

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:

License
-------

Licensed using the `Apache 2.0 license <https://www.apache.org/licenses/LICENSE-2.0>`_.

    Copyright 2023 Advanced Micro Devices, Inc
    
  `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

Trademark Notice
----------------

    Xilinx, the Xilinx logo, Artix, ISE, Kintex, Spartan, Virtex, Zynq, and
    other designated brands included herein are trademarks of Xilinx in the
    United States and other countries.
    
    All other trademarks are the property of their respective owners.


