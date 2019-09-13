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

.. _guide-dynamic-eval:

********************************************************
Internals of Dynamic Evaluation
********************************************************

.. toctree::
   :hidden:
   :maxdepth: 1

This document describes the structure and execution of dynamic evaluation module,
implemented as :ref:`dynamicEval <cid-xf::database::dynamicEval>` function.

The structure of this 4-stream input primitve is describe as below.

.. image:: /images/dynamic_eval_top.png
   :alt: Dynamic Evaluation Top Structure
   :width: 80%
   :align: center

As shown in the picture, there are two types of cell design.

For cell1-cell4, the two input which are stream and constant respectively:

.. image:: /images/dynamic_eval_l1_cell.png
   :alt: Dynamic Evaluation Cell 1-4 Structure
   :width: 80%
   :align: center

While Cell 5-Cell 7 have more inputs to select:

.. image:: /images/dynamic_eval_l2_cell.png
   :alt: Dynamic Evaluation Cell 5-7 Structure
   :width: 80%
   :align: center

The configuration of the primitive is defined as below, and the bits
are concatenated without padding from top to bottom in LSB to MSB order.

+----------+--------------+--------+
| Type     | Usage        | Size   |
+==========+==============+========+
| Operator | Output Mux   | 1 bit  |
|          +--------------+--------+
|          | Strm Empty   | 4 bit  |
|          +--------------+--------+
|          | Cell1 OP     | 4 bit  |
|          +--------------+--------+
|          | Cell2 OP     | 4 bit  |
|          +--------------+--------+
|          | Cell3 OP     | 4 bit  |
|          +--------------+--------+
|          | Cell4 OP     | 4 bit  |
|          +--------------+--------+
|          | Cell5 OP     | 4 bit  |
|          +--------------+--------+
|          | Cell6 OP     | 4 bit  |
|          +--------------+--------+
|          | Cell7 OP     | 4 bit  |
+----------+--------------+--------+
| Operand  | C1           | 64 bit |
|          +--------------+--------+
|          | C2           | 64 bit |
|          +--------------+--------+
|          | C3           | 64 bit |
|          +--------------+--------+
|          | C4           | 64 bit |
+----------+--------------+--------+

To automatically generating its configuration, please refer to the software API defined in ``sw_api`` folder.
