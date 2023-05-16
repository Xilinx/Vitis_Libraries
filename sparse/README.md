# Vitis SPARSE Library

Vitis SPARSE Library accelerates basic linear algebra functions for handling sparse matrices.

Vitis SPARSE Library is an open-sourced Vitis library written in C++ and released under
[Apache 2.0 license](https://www.apache.org/licenses/LICENSE-2.0)
for accelerating sparse linear algebra functions in a variety of use cases.

The main target audience of this library is users who want to accelerate
sparse matrix vector multiplication (SpMV) with FPGA cards.
Currently, this library offers two levels of acceleration:

* At module level is for the C++ implementation of basic components used in SpMV functions. These implementations are intended to be used by HLS (High Level Synthesis) users to build FPGA logic for their applications. 
* The kernel level is for pre-defined kernels that are the C++ implementation of SpMV functions. These implementations are intended to demonstrate how FPGA kernels are defined and how L1 primitive functions can be used by any Vitis users to build their kernels for their applications. 
Check the [comprehensive HTML document](https://docs.xilinx.com/r/en-US/Vitis_Libraries/sparse/index.html) for more details.

## Requirements

### FPGA Accelerator Card

Modules and APIs in this library work with Alveo U280 card.

* [Alveo U280](https://www.xilinx.com/products/boards-and-kits/alveo/u280.html)

### Software Platform

Supported operating systems are RHEL/CentOS 7.4, 7.5 and Ubuntu 16.04.4 LTS, 18.04.1 LTS.

_GCC 5.0 or above_ is required for C++11/C++14 support.
With CentOS/RHEL 7.4 and 7.5, C++11/C++14 should be enabled via
[devtoolset-6](https://www.softwarecollections.org/en/scls/rhscl/devtoolset-6/).

### Development Tools

This library is designed to work with Vitis 2022.2,
and a matching version of XRT should be installed.

## Source Files and Application Development
Vitis libraries are organized into L1, L2, and L3 folders, each relating to a different stage of application development.

**L1**:
      Makefiles and sources in L1 facilitate HLS based flow for quick checks. Tasks at this level include:

* Check the functionality of an individual kernel (C-simulation)
* Estimate resource usage, latency, etc. (Synthesis)
* Run cycle accurate simulations (Co-simulation)
* Package as IP and get final resource utilization/timing details (Export RTL)
       
	**Note**:  Once RTL (or XO file after packaging IP) is generated, the Vivado flow is invoked for XCLBIN file generation if required.

**L2**: Makefiles and sources in L2 facilitate building XCLBIN file from various sources (HDL, HLS or XO files) of kernels with host code written in OpenCL/XRT framework targeting a device. This flow supports:

* Software emulation to check the functionality
* Hardware emulation to check RTL level simulation
* Build and test on hardware

**L3**: Makefiles and sources in L3 demonstrate applications developed involving multiple kernels in pipeline. These Makefiles can be used for executing tasks, as with the L2 Makefiles.

## Benchmark Result

In `L2/benchmarks`, more details about the benchmarks, please kindly find them in [benchmark results](https://docs.xilinx.com/r/en-US/Vitis_Libraries/sparse/benchmark/spmv_double.html).

## Documentations
For more details of the sparse library, please refer to [sparse Library Documentation](https://docs.xilinx.com/r/en-US/Vitis_Libraries/sparse/index.html).

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
