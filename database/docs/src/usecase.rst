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

.. _use_case:

Typical Use Cases
=================

Database acceleration can be achieved in different granularity and flexibility, and this library supports the following use cases:

+--------------------------------+----------------------------------------------------------------------------------------------+
| Acceleration Scope             | Developer's Usage of Database Library                                                        |
+================================+==============================================================================================+
| One hot SQL query              | Write a query-specific kernel with modules from the library.                                 |
+--------------------------------+----------------------------------------------------------------------------------------------+
| Specific query step            | Write a dedicated kernel for some specific execution step [1]_, like hashing.                |
+--------------------------------+----------------------------------------------------------------------------------------------+
| General SQL query acceleration | Use or derive predefined kernels from library to offload matching execution step(s) to FPGA. |
+--------------------------------+----------------------------------------------------------------------------------------------+

Developers targeting the first or second use case may reference the :ref:`module_guide` section,
while those aim to accelerate database on general cases could find the most related info in :ref:`gqe_guide` section.

Developers looking for fast FPGA adoption in database without going into hardware details should look into the
:ref:`gqe_l3_guide`, in which we will present our overlay with pure software APIs.

.. NOTE::
   L3 Overlay is still under active development now. Stay tuned!


.. [1] Execution plan is an ordered list of primitive steps to execute an SQL query in a relational database.
