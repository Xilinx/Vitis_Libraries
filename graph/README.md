# Vitis Graph Library

Vitis Graph Library is an open-sourced Vitis library written in C++ for accelerating graph applications in a variety of use cases. It now covers a level of acceleration: the module level (L1), the pre-defined kernel level (L2) and the asynchronous software level (L3). 

## Overview

The algorithms implemented by Vitis Graph Library include:

- Similarity analysis: Cosine Similarity, Jaccard Similarity.
- Classification: k-nearest Neighbor, maximal independent set.
- Centrality analysis: PageRank.
- Pathfinding: Single Source Shortest Path (SSSP), Multi-Sources Shortest Path (MSSP), Minimum Spanning Tree and Estimated Diameter.
- Connectivity analysis: Weakly Connected Components and Strongly Connected Components.
- Community Detection: Louvain Modularity (From 22.1, Louvain API can support large-scale graphs), Label Propagation and Triangle Count.
- Search: Breadth First Search and 2-Hop Search.
- Graph Format: Renumber, Calculate Degree and Format Convertion between CSR and CSC.

## Source Files and Application Development
Vitis libraries are organized into L1, L2, and L3 folders, each relating to a different stage of application development.

**L1** :
      Makefiles and sources in L1 facilitate HLS based flow for quick checks. Tasks at this level include:

* Check the functionality of an individual kernel (C-simulation)
* Estimate resource usage, latency, etc. (Synthesis)
* Run cycle accurate simulations (Co-simulation)
* Package as IP and get final resource utilization/timing details (Export RTL)
       
	**Note**:  Once RTL (or XO file after packaging IP) is generated, the Vivado flow is invoked for XCLBIN file generation if required.

**L2** :
       Makefiles and sources in L2 facilitate building XCLBIN file from various sources (HDL, HLS or XO files) of kernels with host code written in OpenCL/XRT framework targeting a device. This flow supports:

* Software emulation to check the functionality
* Hardware emulation to check RTL level simulation
* Build and test on hardware

**L3** :
       Makefiles and sources in L3 demonstrate applications developed involving multiple kernels in pipeline. These Makefiles can be used for executing tasks, as with the L2 Makefiles.

## Benchmark Result

In `L2/benchmarks`, these kernels are built into xclbins targeting Alveo U250/U50. We achieved a good performance on several dataset. For more details about the benchmarks, please find them in [benchmark results](https://docs.xilinx.com/r/en-US/Vitis_Libraries/graph/benchmark.html).

## Software level API

`L3` offers asynchronous software level APIs. The L3 framework can fully use the hardware resources and achieve high throughput scheduling. And users can send multiple requests at the same time. For details on running these cases, please refer to [Vitis Graph Library Documentation](https://docs.xilinx.com/r/en-US/Vitis_Libraries/graph/guide_L3/L3_internal/getting_started.html).

## Get Started

To get started with the Vitis Graph Library, please have a visit on our [tutorial page](https://docs.xilinx.com/r/en-US/Vitis_Libraries/graph/tutorial.html).

## Documentations

For more details of the Graph library, please refer to [Vitis Graph Library Documentation](https://docs.xilinx.com/r/en-US/Vitis_Libraries/graph/index.html).

## License

Licensed using the [Apache 2.0 license](https://www.apache.org/licenses/LICENSE-2.0).

    Copyright (C) 2019-2022, Xilinx, Inc.
    Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
    
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
    
        http://www.apache.org/licenses/LICENSE-2.0
    
    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
    Copyright (C) 2019-2022, Xilinx, Inc.
    Copyright (C) 2022-2023, Advanced Micro Devices, Inc.



