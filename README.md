# XF Database Library

XF Database Library is an open-sourced Vitis library written in C++ for
accelerating database applications in a variety of use cases.
It now covers two levels of acceleration: the module level and the pre-defined kernel level,
and will evolve to offer the third level as pure software APIs working with pre-defined hardware overlays.

* At module level, it provides optimized hardware implementation of most common relational database execution plan steps,
  like hash-join and aggregation.
* In kernel level, the post-bitstream-programmable kernel can be used to map a sequence of execution plan steps,
  without having to compile FPGA binaries for each query.
* The upcoming software API level will wrap the details of offloading acceleration with prebuilt binary (overlay)
  and allow users to accelerate supported database tasks on Alveo cards without hardware development.

Since all the kernel code is developed in HLS C++ with the permissive Apache 2.0 license,
advanced users can easily tailor, optimize or combine with property logic at any levels.
Demo/examples of different database acceleration approach are also provided with the library for easy on-boarding.

Check the [comprehensive HTML document](#) for more details.

## Requirements

### Software Platform

This library is designed to work with Vitis 2019.2 and later, and therefore inherits the system requirements of Vitis and XRT.

Supported operating systems are RHEL/CentOS 7.4, 7.5 and Ubuntu 16.04.4 LTS, 18.04.1 LTS.
With CentOS/RHEL 7.4 and 7.5, C++11/C++14 should be enabled via
[devtoolset-6](https://www.softwarecollections.org/en/scls/rhscl/devtoolset-6/).

### PCIE Accelerator Card

Hardware modules and kernels are designed to work with 16nm Alveo cards. GQE kernels are best tuned for U280, and could be tailored for other devices.

## Benchmark Result

By offloading four joins of `customer`, `orders`, `lineitem` and `supplier` tables
in Query 5 from industry-recognized TPC-H benchmark to FPGA, we achieved 7.8x speedup against
state-of-art commercial relational database running on a 16-core cloud instance.

For detailed information, please reference to the benchmark section of document.


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

