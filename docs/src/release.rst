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

.. _release_note:

Release Note
============

.. toctree::
   :hidden:
   :maxdepth: 1

2.0
----

The 2.0 release introduces GQE (generic query engine) kernels,
which are post-bitstream programmable
and allow different SQL queries to be accelerated with one xclbin.
It is conceptually a big step from per-query design,
and a sound example of Xilinx's acceleration approach.

Each GQE kernel is essentially a programmable pipeline of
execution step primitives, which can be enabled or bypassed via run-time configuration.

1.0
----

The 1.0 release provides a range of HLS primitives for mapping the execution
plan steps in relational database. They cover most of the occurrence in the
plan generated from TPC-H 22 queries.

These modules work in streaming fashion and can work in parallel
when possible.
