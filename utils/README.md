# Vitis Utility Library

Vitis Utility Library is an open-sourced Vitis library of common patterns of streaming and storage access.
It aims to assist developers to efficiently access memory in DDR, HBM or URAM, and perform data distribution, collection,
reordering, insertion, and discarding along stream-based transfer.

Check the [comprehensive HTML document](https://docs.xilinx.com/r/en-US/Vitis_Libraries/utils/index.html) for more details.

## Requirements

### Software Platform

Supported operating systems are RHEL8.10, RHEL9.2,RHEL9.3,RHEL9.4,RHEL9.5 and Ubuntu 22.04.3 LTS, 22.04.4 LTS, 22.04.5 LTS.

And C++14 should be enabled during compilation.

### Development Tools

This library is designed to work with Vitis 2022.2 and later,
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

## Design Flows

The common tool and library pre-requisites that apply across all design flows are documented in the requirements section above.

Recommended design flow is shown as follows:


### Shell Environment

Setup the build environment using the Vitis script, and set the installation folder of platform files via `PLATFORM_REPO_PATHS` variable.

```console
source /opt/xilinx/2025.2/Vitis/settings64.sh
export PLATFORM_REPO_PATHS=/opt/xilinx/platforms
```

Setting the `PLATFORM_REPO_PATHS` to installation folder of platform files can enable makefiles in this library to use the `PLATFORM` variable as a pattern.
Otherwise, full path to .xpfm file needs to be provided through the `PLATFORM` variable.

### Running HLS cases

L1 provides the modules to work distribution and result collection in different algorithms, manipulate streams:
including combination, duplication, synchronization, and shuffle, updates URAM array in tighter initiation internal (II).

The recommend flow to evaluate and test L1 components is described as follows using Vivado HLS tool.
A top level C/C++ testbench (typically `algorithm_name.cpp`) prepares the input data, passes them to the design under test,
then performs any output data post processing and validation checks.

A Makefile is used to drive this flow with `make run TARGET=<TARGET> PLATFORM=<PLATFORM>`

`TARGET` can be any of the following values:

- `csim` (high level simulation)
- `csynth` (high level synthesis to RTL)
- `cosim` (cosimulation between software testbench and generated RTL)
- `vivado_syn` (synthesis by Vivado)
- `vivado_impl` (implementation by Vivado)

The flow is launched from the shell by calling `make` with variables set as in the example below:

```console
    . /opt/xilinx/xrt/setup.sh
    export PLATFORM_REPO_PATHS=/opt/xilinx/platforms
    cd L1/tests/specific_algorithm/
    make run TARGET=csim  PLATFORM=u250_xdma_201830_1 # Only run C++ simulation on U250 card
```

To enable more than C++ simulation, just switch other steps to `1` in `make` command line.
The default value of all these control variables are ``0``, so they can be omitted from command line
if the corresponding step is not wanted.

As well as verifying functional correctness, the reports generated from this flow give an indication of logic utilization,
timing performance, latency and throughput.
The output files of interest can be located at the location of the test project where the path name is "test.prj".

## License

Licensed using the [Apache 2.0 license](https://www.apache.org/licenses/LICENSE-2.0).

    Copyright (C) 2019-2022, Xilinx, Inc.
    Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
    
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
    
        http://www.apache.org/licenses/LICENSE-2.0
    
    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

