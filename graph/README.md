# Vitis Graph Library

Vitis Graph Library is an open-sourced Vitis library written in C++ for accelerating graph applications in a variety of use cases. It now covers a level of acceleration: the pre-defined kernel level (L2), and will evolve to offer the module level (L1) and the software API level (L3).

## Overview

The algorithms implemented by Vitis Graph Library include:

- Centrality analysis: Page Rank.
- Pathfinding:  Single Source Shortest Path.
- Connectivity analysis: Weekly Connected Components and Strongly Connected Components.
- Community Detection:  Label Propagation and Triangle Count.
- Search: Breadth First Search.
- Graph Format: Calculate Degree and Format Convert between CSR and CSC.

## Benchmark Result

In `L2/benchmarks`, these Kernels are combined into xclbins and compared with Spark. For details on running these cases, please refer to the README file in that folder.

## License

Licensed using the [Apache 2.0 license](https://www.apache.org/licenses/LICENSE-2.0).

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
    Copyright 2019 Xilinx, Inc.

## Contribution/Feedback

Welcome! Guidelines to be published soon.

