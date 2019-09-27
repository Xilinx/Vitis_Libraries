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

.. _gqe_overview:

******************************************
GQE Overview
******************************************

.. toctree::
   :hidden:
   :maxdepth: 2

As explained in the :ref:`use_case` section, Generic Query Engine (GQE) is our
solution for general acceleration in database.

The GQE consists of a hardware overlay of post-bitstream programmable
kernel(s) and corresponding software stack.
It can process different tasks with the same xclbin downloaded into Alveo card.

.. image:: /images/gqe_overview.png
   :alt: General Query Engine Overview
   :scale: 80%
   :align: center

In Level 2 acceleration, we provide the GQE's kernel(s) as documented APIs,
so that advanced developers can mix or extend the kernels for their own design.
