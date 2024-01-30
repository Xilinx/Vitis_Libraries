.. 
   .. Copyright © 2021–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

********************************
Louvain Partition Demo
********************************

To get scalability for graph size and high-level concurrency for multi-compute units on multi boards, louvainPartition API with two partition methods is provided with no communication between subgraphs processing. 

* Linear louvain partition method, simply dived vertexes linearly, and saves the connection edges between subgraphs to ghost edges. So it is faster in low bandwidth graph but results more ghost edges.
* BFS louvain partition method, dived vertexes by BFS method, and saves the connection edges between subgraphs to ghost edges. Its performance of modularity result keeps the same level between high and low bandwidth input graph. 

Linear partition achieve on the high bandwidth and low bandwidth graph is shown in the following figure. Linear partition is not suitable for High bandwidth graph.

.. image:: /images/louvainlinearpartition.PNG
   :alt: Figure 1 Linear partition achieve on the high bandwidth and low bandwidth graph
   :width: 80%
   :align: center

In this demo, two methods can be switched by corresponding commands. The comparison of input and output is shown in the following table.

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

Ensure to run the script L3/tests/pre_launch.sh to set the path of libgraphL3.so

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

Ensure to run the script L3/tests/pre_launch.sh to set the path of libgraphL3.so

.. code-block:: sh

	cd L3/tests/louvainPartition
	make host
	./build_dir.sw_emu.xilinx_u55c_gen3x16_xdma_2_202110_1/host.exe ./data/example-wt.txt -kernel_mode 2 -num_pars 2 -create_alveo_BFS_partitions -name example_tx
