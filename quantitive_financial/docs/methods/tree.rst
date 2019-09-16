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


*************************************************
Internal Design of Tree Lattice 
*************************************************


Overview
========
Tree Lattice is among the most commonly used tools to price options in financial mathematics.

In general the approach is to divide time between now and the option's expiration into N discrete periods. At the specific time n, the model has a finite number of outcomes at time n + 1 such that every possible change in the state of the world between n and n + 1 is captured in a branch. This process is iterated until every possible path between n = 0 and n = N is mapped. Probabilities are then estimated for every n to n + 1 path. The outcomes and probabilities flow backwards through the tree until a fair value of the option today is calculated (from viki) .


Implemention
============
This framework in L1 as shown in Figure 1 that is a trinomial tree based lattice, where the Rate Model supports multiple models, including Vasicek, HullWhite, BlackKarasinski, CoxIngersollRoss, ExtendedCoxIngersollRoss, Two-additive-factor gaussian, and the Instrument supports multiple Instruments, including swaption, swap, capfloor, callablebond. The process is as follows:


.. _my-figure1:
.. figure:: /images/tree/treeFramework.png
    :alt: Figure 1 Tree Lattice architecture on FPGA
    :width: 80%
    :align: center


1. The Setup module of the framework is based on the structure of the tree and Interest Rate Model, and the floating interest rates and tree related parameters are calculated from 0 to N time point by time point to prepare the interest rates and the tree related parameters for subsequent calculations. Since the dataflow of the framework is progressively accumulated or decreases with time, the tree structure can calculate the values of the corresponding nodes at each time point without calculating the values of all nodes in advance, thus reducing the storage resources to the greatest extent.
2. The Rollback module using the same structure of the tree and discount from the Rate Model to obtain the values of each tree node from N to 0 time point by time point. When the timepoint is 0, the value is NPV. The implementation is shown in Figure 2, where arrows indicate data flow.


.. _my-figure2:
.. figure:: /images/tree/rollback.png
    :alt: Figure 2 rollback module architecture on FPGA
    :width: 80%
    :align: center

