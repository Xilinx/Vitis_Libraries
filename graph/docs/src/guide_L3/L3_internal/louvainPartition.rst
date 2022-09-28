.. 
   Copyright 2021 Xilinx, Inc.
  
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
  
       http://www.apache.org/licenses/LICENSE-2.0
  
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

********************************
Louvain Paritition Demo
********************************

To get scalability for graph size and get high-level concurrency for multi compute units on multi boards, we provided louvainPartition API with two partition methods which no communication between subgraphs processing. 

* Linear louvain partition mothed, simply dived vertexes linearly, and save the connection edges between subgraphs to ghost edges. So it is faster in low bandwidth graph but result more ghost edges.
* BFS louvain partition mothed, dived vertexes by BFS method, and save the connection edges between subgraphs to ghost edges. Its performance of modularity result keeps the same level between high and low bandwidth input graph. 

Linear partition achieve on the high bandwidth and low bandwidth graph is shown as the Figure 1. Linear partition is not suitable for High bandwidth graph.

.. image:: /images/louvainlinearpartition.PNG
   :alt: Figure 1 Linear partition achieve on the high bandwidth and low bandwidth graph
   :width: 80%
   :align: center

In this demo, two methods can be switched by corresponding commands. The comparison of input and output is shown as the table 1.

.. table:: Table 1 input and output comparison for different partition algorithms
    :align: center

    +-------------------+----------------------------+---------------------------------+
    |                   |      Linear paritition     |          BFS partition          |
    +-------------------+----------------------------+---------------------------------+
    | input graph       | *.mtx, *.txt ...           | *.mtx, *.txt ...                |
    +-------------------+----------------------------+---------------------------------+
    | input commend     |  -create_alveo_partitions  |  -create_alveo_BFS_partitions   |
    +-------------------+----------------------------+---------------------------------+
    | output project    | name.par.proj              | name.par.proj                   |
    +-------------------+----------------------------+---------------------------------+
    | output header file| *.par.src, *.par.parlv     | *.par.src, *.par.parlv *.bfs.adj|
    +-------------------+----------------------------+---------------------------------+
    | output subgraph   | name_000.par ...           |  name_000.par ...               |
    +-------------------+----------------------------+---------------------------------+

Linear Louvain Partition Flow
##########################################

Ensure run the script L3/tests/pre_launch.sh to set the path of libgraphL3.so

.. code-block:: sh

	cd L3/tests/louvainPartition
	make host
	./build_dir.sw_emu.xilinx_u55c_gen3x16_xdma_2_202110_1/host.exe ./data/example-wt.txt -kernel_mode 2 -num_pars 2 -create_alveo_partitions -name example_tx

Louvain fast Input Arguments:

.. code-block:: sh

   Usage: host.exe -[-kernel_mode -num_pars -create_alveo_partitions -create_alveo_BFS_partitions -name]
         -kernel_mode:                 the kernel mode  : 1 is u50 1 cu, 2 is u55c 2cu parallel launch in louvainRunSubGraph
         -num_pars:                    partition number : 2~n
         -create_alveo_partitions:     Linear partition flow
         -create_alveo_BFS_partitions: BFS partition flow
         -name:                        name of subgraph : name

BFS Louvain Partition Flow
##########################################

Ensure run the script L3/tests/pre_launch.sh to set the path of libgraphL3.so

.. code-block:: sh

	cd L3/tests/louvainPartition
	make host
	./build_dir.sw_emu.xilinx_u55c_gen3x16_xdma_2_202110_1/host.exe ./data/example-wt.txt -kernel_mode 2 -num_pars 2 -create_alveo_BFS_partitions -name example_tx
