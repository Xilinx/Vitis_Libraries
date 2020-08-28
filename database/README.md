# Vitis Database Library

Vitis Database Library is an open-sourced Vitis library written in C++ for
accelerating database applications in a variety of use cases.
It now covers two levels of acceleration: the module level and the pre-defined kernel level,
and will evolve to offer the third level as pure software APIs working with pre-defined hardware overlays.

* At module level, it provides an optimized hardware implementation of most common relational database execution plan steps,
  like hash-join and aggregation.
* In kernel level, the post-bitstream-programmable kernel can be used to map a sequence of execution plan steps,
  without having to compile FPGA binaries for each query.
* The upcoming software API level will wrap the details of offloading acceleration with prebuilt binary (overlay)
  and allow users to accelerate supported database tasks on Alveo cards without hardware development.

Since all the kernel code is developed in HLS C++ with the permissive Apache 2.0 license,
advanced users can easily tailor, optimize or combine with property logic at any levels.
Demo/examples of different database acceleration approach are also provided with the library for easy on-boarding.

Check the [comprehensive HTML document](https://xilinx.github.io/Vitis_Libraries/database/2020.1/) for more details.

## Requirements

### Software Platform

This library is designed to work with Vitis 2020.1 and later, and therefore inherits the system requirements of Vitis and XRT.

Supported operating systems are RHEL/CentOS 7.4, 7.5 and Ubuntu 16.04.4 LTS, 18.04.1 LTS.
With CentOS/RHEL 7.4 and 7.5, C++11/C++14 should be enabled via
[devtoolset-6](https://www.softwarecollections.org/en/scls/rhscl/devtoolset-6/).

### PCIE Accelerator Card

All the modules and APIs works with Alveo U280 out of the box, many support U250 and U200 as well. Most of the APIs can be scaled and tailored for any 16nm Alveo card.

* [Alveo U280](https://www.xilinx.com/products/boards-and-kits/alveo/u280.html#gettingStarted)
* [Alveo U250](https://www.xilinx.com/products/boards-and-kits/alveo/u250.html#gettingStarted)
* [Alveo U200](https://www.xilinx.com/products/boards-and-kits/alveo/u200.html#gettingStarted)

### Shell Environment

Setup the build environment using the Vitis and XRT scripts.

```console
source /opt/xilinx/Vitis/2020.1/settings64.sh
source /opt/xilinx/xrt/setup.sh
export PLATFORM_REPO_PATHS=/opt/xilinx/platforms
```

Setting `PLATFORM_REPO_PATHS` to the installation folder of platform files can enable makefiles
in this library to use `DEVICE` variable as a pattern.
Otherwise, full path to .xpfm file needs to be provided via `DEVICE` variable.

### Dependency

This library depends on the Vitis Utility Library, which is assumed to be placed in the same path as this library with name `utils`. Hence the directory is organized as follows.

```
/cloned/path/database # This library, which contains L1, L2, etc.
/cloned/path/utils # The Vitis Utility Library, which contains its L1.
```

## Design Flows

Recommended design flows are categorised by the target level:

* L1
* L2

The common tool and library pre-requisites that apply across all design flows are documented in the requirements section above.

### L1

L1 provides the basic modules could be used to build GQE kernels.

The recommend flow to evaluate and test L1 components is described as follows using Vivado HLS tool.
A top level C/C++ test bench (typically `algorithm_name.cpp`) prepares the input data, passes them to the design under test, then performs output data post processing and validation checks.

A Makefile is used to drive this flow with available steps including `CSIM` (high level simulation), `CSYNTH` (high level synthesis to RTL) and `COSIM` (co-simulation between software test bench and generated RTL), `VIVADO_SYN` (synthesis by Vivado), `VIVADO_IMPL` (implementation by Vivado). The flow is launched from the shell by calling `make` with variables set as in the example below:

```console
cd L1/tests/specific_algorithm/
make run CSIM=1 CSYNTH=0 COSIM=0 VIVADO_SYN=0 VIVADO_IMPL=0 \
    DEVICE=/path/to/xilinx_u280_xdma_201910_1.xpfm
```

As well as verifying functional correctness, the reports generated from this flow give an indication of logic utilization, timing performance, latency and throughput. The output files of interest can be located at the location of the test project where the path name is correlated with the algorithm. i.e. the callable function within the design under test.

### L2

L2 provides the Generic Query Engine (GQE) kernels.

The available flow for L2 based around the Vitis tool facilitates the generation and packaging of GQE kernels along with the required host application for configuration and control. In addition to supporting FPGA platform targets, emulation options are available for preliminary investigations, or where dedicated access to a hardware platform may not be available. Two emulation options are available, the software emulation which performs a high level simulation of the design, and the hardware emulation which performs a cycle-accurate simulation of the generated RTL for the kernel. This flow is makefile-driven from the console where the target is selected by a command line parameter as in the example below:

```console
cd L2/tests/specific_GQE_kernel

# build and run one of the following using U280 platform
#  * software emulation
#  * hardware emulation,
#  * actual deployment on physical platform

make run TARGET=sw_emu DEVICE=/path/to/xilinx_u280_xdma_201910_1.xpfm
make run TARGET=hw_emu DEVICE=/path/to/xilinx_u280_xdma_201910_1.xpfm
make run TARGET=hw DEVICE=/path/to/xilinx_u280_xdma_201910_1.xpfm

# delete all xclbin and host binary
make cleanall
```

The outputs of this flow are packaged kernel binaries (xclbin files) that can be downloaded to the FPGA platform, and host executables to configure and coordinate data transfers. The output files of interest can be located where the file names are correlated with the specific query.

This flow can be used to verify functional correctness in hardware and measure actual performance.

## Library API Summary

### L1

| Library API             | Description                                                                                                                   |
|-------------------------|-------------------------------------------------------------------------------------------------------------------------------|
| aggregate               | A group of overloaded aggregate functions, supports SUM, MAX, MIN, MEAN, VARIANCE, COUNT, COUNTNONZERO operation.             |
| bitonicSort             | Bitonic sort is a parallel algorithm for sorting.                                                                             |
| bfGen                   | Generate the bloom-filter in on-chip RAM blocks.                                                                              |
| bfGenStream             | Generate the bloom-filter in on-chip RAM blocks, and emit the vectors through FIFO upon finish.                               |
| bfCheck                 | Check existence of a value using bloom-filter vectors.                                                                        |
| combineCol              | A group of overloaded functions for combining two to five columns into one.                                                   |
| splitCol                | A group of overloaded functions for splitting previously combined column into two to five separate ones.                      |
| directGroupAggregate    | Group-by aggregation with limited key width.                                                                                  |
| duplicateCol            | Duplicate one column into two columns.                                                                                        |
| dynamicEval             | Dynamic expression evaluation.                                                                                                |
| dynamicFilter           | A group overloaded functions for filtering payloads according to one to four conditions columns pro gamed at during run-time. |
| groupAggregate          | A series of overloaded functions for group-aggregation. Input rows are required to be sorted by grouping key(s).              |
| hashAntiJoin            | Hash-Anti-Join primitive.                                                                                                     |
| hashGroupAggregate      | Generic hash group aggregate primitive.                                                                                       |
| hashJoinMPU             | Hash-Join primitive, using multiple DDR/HBM buffers.                                                                          |
| hashJoinV3              | Hash-Join v3 primitive, it is designed for HBM device and performs better in large size of table.                             |
| hashBuildProbeV3        | Hash-Build-Probe v3 primitive, it can perform hash build and hash probe separately.                                           |
| hashJoinV4              | Hash-Join v4 primitive, using bloom-filter to enhance performance of hash join, designed for HBM device.                      |
| hashBuildProbeV4        | Hash-Build-Probe v4 primitive, build and probe are separately performed. This primitive is designed for HBM device only.      |
| hashLookup3             | Lookup3 algorithm generates 64-bit or 32-bit hash.                                                                            |
| hashMultiJoin           | Hash-Multi-Join primitive is based on hashJoinV3, and can be programmed at run-time to perform inner, anti or left join.      |
| hashMurmur3             | Murmur3 hash algorithm.                                                                                                       |
| hashPartition           | Hash-Partition primitive splits a table into partitions of rows based on hash of a selected key.                              |
| hashSemiJoin            | Hash-Semi-Join primitive is based on hashJoinMPU, but performs semi-join.                                                     |
| insertSort              | Insert sort algorithm on chip.                                                                                                |
| mergeJoin               | Merge join algorithm for sorted tables without duplicated keys in the left table.                                             |
| mergeLeftJoin           | Merge left join function for sorted tables, the left table should not have duplicated keys.                                   |
| mergeSort               | Merge sort algorithm.                                                                                                         |
| nestedLoopJoin          | Nested loop join.                                                                                                             |
| scanCmpStrCol           | Scan multiple string columns in global memory, and compare each of them with a constant string                                |
| scanCol                 | A group of overloaded functions for Scanning 1 to 6 columns as a table from DDR/HBM buffers.                                  |
| staticEval              | A group of overloaded functions for evaluating a compile-time selected expression on each row with one to four columns.       |


### L2

| Library API | Description          |
|-------------|----------------------|
| gqeAggr     | GQE aggregate kernel |
| gqeJoin     | GQE join kernel      |
| gqePart     | GQE partition kernel |


## Benchmark Result

In L1, the library provides TPC-H Query 5 and the modified version of Query 6 as hard-coded kernels. They demonstrate hardening SQL queries with L1 primitives. Corresponding projects can be found in `L1/demos`.

A list of Vitis projects can be found in `L1/benchmarks`. They are provided to help users evaluate the performance of most critical primitives.

In `L2/demos`, GQE Kernels are combined into xclbins, and the host code shows how TPC-H queries of different scale factors can be accelerated with the same one or two xclbins. For details on running these cases, please refer to the README file in that folder.

## License

Licensed using the [Apache 2.0 license](https://www.apache.org/licenses/LICENSE-2.0).

    Copyright 2019-2020 Xilinx, Inc.
    
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
    
        http://www.apache.org/licenses/LICENSE-2.0
    
    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

## Contribution/Feedback

Welcome! Guidelines to be published soon.

