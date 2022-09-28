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
Internal Design of Louvain Modularity
*************************************************


Overview
========
The Louvain method for community detection is a method to extract communities from large networks created by Blondel from the University of Louvain (the source of this method's name). The method is a greedy optimization method that appears to run in time O(n \cdot log n), if n is the number of nodes in the network.
Since we release the louvain kernel on FPGA from 21.1, the performance of 1 compute unit in this kernel is improved from 18x, to 33x, to 65x vs CPU.
The latest design cloud achieve 2 compute units in u55c by Vitis 22.1, which means the max speed up by u55c cloud be 2*65x vs CPU.
Now the L2 API cloud achieve the entire graph louvain modularity directly. when the graph is less than 128M vertexes and 128M edges by 1 cu on u55c(64M is the limit by 1 cu on u50).
If the graph is larger,  we should launch the L3 louvainPartition API, partition the graph to multi pieces, then run the L3 louvainRun API which cloud run louvain algorithm on multi cu on multi board by the support of xrm.
More details cloud be find in L3 API.  

Algorithm
============
Louvain algorithm implementation:

.. code::

    Louvain(G(V, E, W), Cid)
    Q_old := 0
    Q_new := 0
    Cid_new := C_init

    ColorSets = Coloring(V)
    
    while true     // Iterate until modularity gain becomes negligible.
        for each node Vk in ColorSets    
            Cid_old := Cid_new

            for each v in Vk
                vCid := Cid_old[v]
                
                for each e in e \cup (v, E, W)
                    maxQ := max(\Delta Q)
                    target := Cid_old[e]
                if maxQ > 0 then
                    Cid_new[v] = target
            
            Q_new := Q(Cid_new)
            if Q_new < Q_old then 
                break
            else
                Q_old = Q_new

    return {Cid_old, Q_old}
============
The input matrix should ensure that the following conditions hold:

1. undirected graph
2. compressed sparse column (COO) format

The algorithm implemention is shown as the figure below:

Figure 1 : Louvain calculate modularity architecture on FPGA

.. _my-figure-Louvain-1:
.. figure:: /images/Louvainfast_kernel.png
      :alt: Figure 1 Louvain calculate modularity architecture on FPGA
      :width: 80%
      :align: center

.. toctree::
   :maxdepth: 1

