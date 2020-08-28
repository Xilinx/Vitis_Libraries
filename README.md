# Vitis Utility Library

Vitis Utility Library is an open-sourced Vitis library of common patterns of streaming and storage access.
It aims to assist developers to efficiently access memory in DDR, HBM or URAM, and perform data distribution, collection,
reordering, insertion, and discarding along stream-based transfer.

Check the [comprehensive HTML document](https://xilinx.github.io/Vitis_Libraries/utils/2020.1/) for more details.

## Requirements

### Software Platform

This library is designed to work with Vitis 2020.1 and later, and therefore inherits the system requirements of Vitis and XRT.

Supported operating systems are RHEL/CentOS 7.4, 7.5 and Ubuntu 16.04.4 LTS, 18.04.1 LTS.
With CentOS/RHEL 7.4 and 7.5, C++11/C++14 should be enabled via
[devtoolset-6](https://www.softwarecollections.org/en/scls/rhscl/devtoolset-6/).

### PCIE Accelerator Card

Modules in this library are designed to work with all Alveo cards.
* [Alveo U200](https://www.xilinx.com/products/boards-and-kits/alveo/u200.html#gettingStarted)
* [Alveo U250](https://www.xilinx.com/products/boards-and-kits/alveo/u250.html#gettingStarted)
* [Alveo U280](https://www.xilinx.com/products/boards-and-kits/alveo/u280.html#gettingStarted)

### Shell Environment

Setup the build environment using the Vitis script, and set the installation folder of platform files via `PLATFORM_REPO_PATHS` variable.

```console
source /opt/xilinx/Vitis/2020.1/settings64.sh
export PLATFORM_REPO_PATHS=/opt/xilinx/platforms
```

Setting the `PLATFORM_REPO_PATHS` to installation folder of platform files can enable makefiles in this library to use the `DEVICE` variable as a pattern.
Otherwise, full path to .xpfm file needs to be provided through the `DEVICE` variable.

## Design Flows

The common tool and library pre-requisites that apply across all design flows are documented in the requirements section above.

Recommended design flow is shown as follows:

### L1

L1 provides the modules to work distribution and result collection in different algorithms, manipulate streams: including combination, duplication, synchronization, and shuffle, updates URAM array in tighter initiation internal (II).

The recommend flow to evaluate and test L1 components is described as follows using Vivado HLS tool.
A top level C/C++ testbench (typically `algorithm_name.cpp`) prepares the input data, passes them to the design under test, then performs any output data post processing and validation checks.

A Makefile is used to drive this flow with available steps including `CSIM` (high level simulation), `CSYNTH` (high level synthesis to RTL) and `COSIM` (cosimulation between software testbench and generated RTL), `VIVADO_SYN` (synthesis by Vivado), `VIVADO_IMPL` (implementation by Vivado). The flow is launched from the shell by calling `make` with variables set as in the example below:

```console
cd L1/tests/specific_algorithm/
make run CSIM=1 CSYNTH=0 COSIM=0 VIVADO_SYN=0 VIVADO_IMPL=0 \
         DEVICE=/path/to/xilinx_u200_xdma_201830_2.xpfm
```

To enable more than C++ simulation, just switch other steps to `1` in `make` command line.

As well as verifying functional correctness, the reports generated from this flow give an indication of logic utilization, timing performance, latency and throughput. The output files of interest can be located at the location of the test project where the path name is "test.prj".

## Library Contents

| Library Function | Description | Layer |
|------------------|-------------|-------|
| axiToMultiStream | Loading multiple categories of data from one AXI master to streams | L1 |
| axiToStream | Loading data elements from AXI master to stream | L1 |
| axiToCharStream | Loading char data from AXI master to stream | L1 |
| streamCombine overload (1) | combine multiple streams into one, shift selected streams to LSB side | L1 |
| streamCombine overload (2) | combine multiple streams into one, shift selected streams to MSB side | L1 |
| streamCombine overload (3) | combine multiple streams into a wide one, align to LSB | L1 |
| streamCombine overload (4) | combine multiple streams into a wide one, align to MSB | L1 |
| streamDiscard overload (1) | Discard multiple streams with end flag helper for each | L1 |
| streamDiscard overload (2) | Discard multiple streams synchronized with one end flag | L1 |
| streamDiscard overload (3) | Discard one stream with its end flag helper | L1 |
| streamDup overload (1) | Duplicate stream | L1 |
| streamDup overload (2) | Duplicate stream | L1 |
| streamNToOne overload (1) | stream distribute, skip to read the empty input streams | L1 |
| streamNToOne overload (2) | stream distribute, skip to read the empty input streams | L1 |
| streamNToOne overload (3) | stream distribute, in round-robin order from NStrm input streams | L1 |
| streamNToOne overload (4) | stream distribute, in round-robin order from NStrm input streams | L1 |
| streamNToOne overload (5) | This function selects from input streams based on tags | L1 |
| streamNToOne overload (6) | This function selects from input streams based on tags | L1 |
| streamOneToN overload (1) | stream distribute, using load-balancing algorithm | L1 |
| streamOneToN overload (2) | stream distribute, using load-balancing algorithm | L1 |
| streamOneToN overload (3) | stream distribute, in round-robin order from first output | L1 |
| streamOneToN overload (4) | stream distribute, in round-robin order from first output | L1 |
| streamOneToN overload (5) | This function send element from one stream to multiple streams based on tags | L1 |
| streamOneToN overload (6) | This function send element from one stream to multiple streams based on tags | L1 |
| streamReorder | Window-reorder in a stream | L1 |
| streamShuffle | Shuffle the contents from an array of streams to another | L1 |
| streamSplit overload (1) | split one wide stream into multiple streams, start from the LSB | L1 |
| streamSplit overload (2) | split one wide stream into multiple streams, start from the MSB | L1 |
| streamSync | Synchronize streams for successor module | L1 |
| streamToAxi | Write elements in burst to AXI master port | L1 |

| Library Class    | Description | Layer |
|------------------|-------------|-------|
| UramArray        | Helper class to create URAM array that can be updated every cycle with forwarding regs | L1 |
| Cache            | Read-only cache to save memory reads from AXI | L1 |


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

Please refer to the wiki for contribution guide lines, and use the issue tab for feedbacks.

