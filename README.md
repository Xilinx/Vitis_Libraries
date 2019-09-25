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

Check the [comprehensive HTML document](#) for more details.

## Library Contents

| Library Function | Description | Layer |
|------------------|-------------|-------|
| aggregate overload (1) | Overload for most common aggregations | L1 |
| aggregate overload (2) | Aggregate function overload for SUM operation | L1 |
| aggregate overload (3) | Aggregate function overload for counting | L1 |
| bitonicSort | Bitonic sort is parallel algorithm for sorting | L1 |
| bfGen | Generate the bloomfilter in on-chip RAM blocks | L1 |
| bfGenStream | Generate the bloomfilter in on-chip RAM blocks, and emit the vectors upon finish | L1 |
| bfCheck | Check existance of value using bloom-filter vectors | L1 |
| combineCol overload (1) | Combines two columns into one | L1 |
| combineCol overload (2) | Combines three columns into one | L1 |
| combineCol overload (3) | Combines four columns into one | L1 |
| combineCol overload (4) | Combines five columns into one | L1 |
| splitCol overload (1) | Split previously combined columns into two | L1 |
| splitCol overload (2) | Split previously combined columns into three | L1 |
| splitCol overload (3) | Split previously combined columns into four | L1 |
| splitCol overload (4) | Split previously combined columns into five | L1 |
| directGroupAggregate overload (1) | Group-by aggregation with limited key width | L1 |
| directGroupAggregate overload (2) | Group-by aggregation with limited key width, runtime programmable | L1 |
| duplicateCol | Duplicate one column into two columns | L1 |
| dynamicEval | Dynamic expression evaluation | L1 |
| dynamicFilter overload (1) | Filter payloads according to conditions set during run-time | L1 |
| dynamicFilter overload (2) | Filter payloads according to conditions set during run-time | L1 |
| dynamicFilter overload (3) | Filter payloads according to conditions set during run-time | L1 |
| dynamicFilter overload (4) | Filter payloads according to conditions set during run-time | L1 |
| groupAggregate overload (1) | group aggregate function that returns same type as input | L1 |
| groupAggregate overload (2) | group aggregate function that returns different type as input | L1 |
| groupAggregate overload (3) | aggregate function that counts and returns uint64_t | L1 |
| groupAggregate overload (4) | aggregate function that counts and returns uint64_t | L1 |
| hashAntiJoin | Multi-PU Hash-Anti-Join primitive, using multiple DDR/HBM buffers | L1 |
| hashGroupAggregate | Generic hash group aggregate primitive | L1 |
| hashJoinMPU overload (1) | Multi-PU Hash-Join primitive, using multiple DDR/HBM buffers | L1 |
| hashJoinMPU overload (2) | Multi-PU Hash-Join primitive, using multiple DDR/HBM buffers | L1 |
| hashJoinV3 | Hash-Join v3 primitive, it takes more resourse than hashJoinMPU and promises a better performance in large size of table | L1 |
| hashBuildProbeV3 | Hash-Build-Probe v3 primitive, it can perform hash build and hash probe separately | L1 |
| hashJoinV4 | Hash-Join v4 primitive, using bloom filter to enhance performance of hash join | L1 |
| hashBuildProbeV4 | Hash-Build-Probe v4 primitive, build and probe are separately performed and controlled by a boolean flag | L1 |
| hashLookup3 overload (1) | lookup3 algorithm, 64-bit hash. II=1 when W<=96, otherwise II=(W/96) | L1 |
| hashLookup3 overload (2) | lookup3 algorithm, 32-bit hash. II=1 when W<=96, otherwise II=(W/96) | L1 |
| hashLookup3 overload (3) | lookup3 algorithm, 64-bit or 32-bit hash | L1 |
| hashMultiJoin | Multi-PU Hash-Multi-Join primitive, using multiple DDR/HBM buffers | L1 |
| hashMurmur3 | murmur3 algorithm | L1 |
| hashPartition | Hash-Partition primitive | L1 |
| hashSemiJoin | Multi-PU Hash-Semi-Join primitive, using multiple DDR/HBM buffers | L1 |
| insertSort | Insert sort top function | L1 |
| mergeJoin | merge join function for sorted tables without duplicated key in the left table | L1 |
| mergeLeftJoin | merge left join function for sorted table, left table should not have duplicated keys | L1 |
| mergeSort | Merge sort function | L1 |
| nestedLoopJoin | nested loop join function | L1 |
| scanCmpStrCol | sacn multiple columns of string in global memory, and compare each of them with constant string | L1 |
| scanCol overload (1) | scan 1 column from DDR/HBM buffers | L1 |
| scanCol overload (2) | scan 2 column from DDR/HBM buffers | L1 |
| scanCol overload (3) | scan 3 column from DDR/HBM buffers | L1 |
| scanCol overload (4) | scan 4 column from DDR/HBM buffers | L1 |
| scanCol overload (5) | scan 5 column from DDR/HBM buffers | L1 |
| scanCol overload (6) | scan 6 column from DDR/HBM buffers | L1 |
| scanCol overload (7) | scan one column from DDR/HBM buffers, emit multiple rows concurrently | L1 |
| scanCol overload (8) | scan two column from DDR/HBM buffers, emit multiple rows concurrently | L1 |
| scanCol overload (9) | scan three column from DDR/HBM buffers, emit multiple rows concurrently | L1 |
| scanCol overload (10) | scan 2 columns from DDR/HBM buffers | L1 |
| scanCol overload (11) | scan 3 columns from DDR/HBM buffers | L1 |
| scanCol overload (12) | scan 4 columns from DDR/HBM buffers | L1 |
| scanCol overload (13) | scan 5 columns from DDR/HBM buffers | L1 |
| staticEval overload (1) | one stream input static evaluation | L1 |
| staticEval overload (2) | two stream input static evaluation | L1 |
| staticEval overload (3) | three stream input static evaluation | L1 |
| staticEval overload (4) | four stream input static evaluation | L1 |
| gqeAggr | GQE Aggr Kernel | L2 |
| gqeJoin | GQE Join Kernel | L2 |
| gqePart | GQE partition kernel | L2 |

## Requirements

### Software Platform

This library is designed to work with Vitis 2019.2 and later, and therefore inherits the system requirements of Vitis and XRT.

Supported operating systems are RHEL/CentOS 7.4, 7.5 and Ubuntu 16.04.4 LTS, 18.04.1 LTS.
With CentOS/RHEL 7.4 and 7.5, C++11/C++14 should be enabled via
[devtoolset-6](https://www.softwarecollections.org/en/scls/rhscl/devtoolset-6/).

### PCIE Accelerator Card

Hardware modules and kernels are designed to work with 16nm Alveo cards. GQE kernels are best tuned for U280, and could be tailored for other devices.

* [Alveo U280](https://www.xilinx.com/products/boards-and-kits/alveo/u280.html#gettingStarted)
* [Alveo U250](https://www.xilinx.com/products/boards-and-kits/alveo/u250.html#gettingStarted)
* [Alveo U200](https://www.xilinx.com/products/boards-and-kits/alveo/u200.html#gettingStarted)

### Shell Environment

Setup the build environment using the Vitis and XRT scripts, and set the PLATFORM_REPO_PATHS to installation folder of platform files.

```console
source /opt/xilinx/Vitis/2019.2/settings64.sh
source /opt/xilinx/xrt/setup.sh
export PLATFORM_REPO_PATHS=/opt/xilinx/platforms
```

## Dependency

This library depends on the Hardware Utility Library, and assumes that library is places in the same path as this library with name `utils`. Hence the directory is organized like following.

```
/cloned/path/database # This library, contains L1, L2, etc.
/cloned/path/utils # The Hardware Utility Library, contains its L1.
```

## Design Flows

Recommended design flows are categorised by the target level:

* L1
* L2

The common tool and library pre-requisites that apply across all design flows are documented in the requirements section above.

### L1

L1 provides the basic modules could be used to build GQE kernels.

The recommend flow to evaluate and test L1 components is described as follows using Vivado HLS tool.
A top level C/C++ testbench (typically `algorithm_name.cpp`) prepares the input data, passes them to the design under test, then performs any output data post processing and validation checks.

A Makefile is used to drive this flow with available steps including `CSIM` (high level simulation), `CSYNTH` (high level synthesis to RTL) and `COSIM` (cosimulation between software testbench and generated RTL), `VIVADO_SYN` (synthesis by Vivado), `VIVADO_IMPL` (implementation by Vivado). The flow is launched from the shell by calling `make` with variables set as in the example below:

```console
cd L1/tests/specific_algorithm/
make run CSIM=1 CSYNTH=0 COSIM=0 VIVADO_SYN=0 VIVADO_IMPL=0 DEVICE=u280_xdma_201910_1
```

As well as verifying functional correctness, the reports generated from this flow give an indication of logic utilization, timing performance, latency and throughput. The output files of interest can be located at the location of the test project where the path name is correlated with the algorithm. i.e. the callable function within the design under test.

### L2

L2 provides the General Query Engine (GQE) kernels.

The available flow for L2 based around the SDAccel tool facilitates the generation and packaging of GQE kernels along with the required host application for configuration and control. In addition to supporting FPGA platform targets, emulation options are available for preliminary investigations or where dedicated access to a hardware platform may not be available. Two emulation options are available, software emulation which performs a high level simulation of the design, and hardware emulation which performs a cycle-accurate simulation of the generated RTL for the kernel. This flow is makefile driven from the console where the target is selected as a command line parameter as in the example below:

```console
cd L2/tests/specific_GQE_kernel

# build and run one of the following using U280 platform
#  * software emulation
#  * hardware emulation,
#  * actual deployment on physical platform

make run TARGET=sw_emu DEVICE=u280_xdma_201910_1
make run TARGET=hw_emu DEVICE=u280_xdma_201910_1
make run TARGET=hw DEVICE=u280_xdma_201910_1

# delete all xclbin and host binary
make cleanall
```

The outputs of this flow are packaged kernel binaries (xclbin files) that can be downloaded to the FPGA platform and host executables to configure and co-ordinate data transfers. The output files of interest can be located at the locations where the file names are correlated with the specific query

This flow can be used to verify functional correctness in hardware and enable actual performance to be measured.

## Benchmark Result

In L1, the library provides TPC-H Query 5 and modified version of Query 6 as hard-coded kernels. They demonstrate hardening SQL queries with L1 primitives. Corresponding projects can be found in `L1/demos`.

A list of Vitis projects can be found `L1/benchmarks`. They are provided to help users to evaluate the performance of most critical primitives.

In `L2/demos`, GQE Kernels are combined into xclbins, and the host code shows how TPC-H queries of different scale factor can be accelerated with the same one or two xclbins. For details on running these cases, please refer to the README file in that folder.

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

