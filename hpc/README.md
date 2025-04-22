# Vitis HPC Library 

Vitis HPC Library provides an acceleration libray for applications with high
computation workload,
e.g.  seismic imaging and inversion, high-precision simulations, genomics and etc.

## Overview

The algorithms implemented by Vitis HPC Library include:

- Multi-Layer Percepton (MLP): 

- 2D Reverse Time Migration (RTM):

- 3D RTM:

- Conjugate Gradient Solvers (CG): 


## Requirements

### FPGA Accelerator Card

Modules and APIs in this library work with Alveo U280 or U250 or U200 or U50.

* [Alveo U280](https://www.xilinx.com/products/boards-and-kits/alveo/u280.html)
* [Alveo U50](https://www.xilinx.com/products/boards-and-kits/alveo/u50.html)
* [Alveo U250](https://www.xilinx.com/products/boards-and-kits/alveo/u250.html)
* [Alveo U200](https://www.xilinx.com/products/boards-and-kits/alveo/u200.html)

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

## Running Test Cases

This library ships two types of case: HLS cases and Vitis cases.
HLS cases can only be found in `L1/tests` folder, and are created to test module-level functionality.

### Setup environment

Setup and build envrionment using the Vitis and XRT scripts:

```
    source <install path>/Vitis/2022.2/settings64.sh
    source /opt/xilinx/xrt/setup.sh
```

### HLS Cases Command Line Flow

```console
cd L1/tests/hls_case_folder
make run CSIM=1 CSYNTH=1 COSIM=1 PLATFORM=<FPGA platform> PLATFORM_REPO_PATHS=<path to platform directories>
```

Test control variables are:

- `CSIM` for high level simulation.
- `CSYNTH` for high level synthesis to RTL.
- `COSIM` for co-simulation between software test bench and generated RTL.

### Vitis Cases Command Line Flow

#### L2

```console
cd L2/tests/vitis_case_folder

make run TARGET=<hw_emu/hw> PLATFORM=<FPGA platform> PLATFORM_REPO_PATHS=<path to platform directories>

# delete generated files
make cleanall
```

#### L3

```console
cd L3/tests/vitis_case_folder

make run TARGET=<hw_emu/hw> PLATFORM=<FPGA platform> PLATFORM_REPO_PATHS=<path to platform directories>

# delete generated files
make cleanall
```

Here, `TARGET` decides the FPGA binary type

- `hw_emu` is for hardware emulation
- `hw` is for deployment on physical card. (Compilation to hardware binary often takes hours.)

Besides ``run``, the Vitis case makefile also allows ``host`` and ``xclbin`` as build target.

## Benchmark Result

In `L2/benchmarks`, these Kernels are built into xclbins targeting Alveo U250, U280 or U50. We achieved a good performance against several problem sizes. For more details about the benchmarks, please kindly find them in [benchmark results](https://docs.xilinx.com/r/en-US/Vitis_Libraries/hpc/benchmark.html). 

## Documentations

For more details of the HPC library, please refer to [Vitis HPC Library Documentation](https://docs.xilinx.com/r/en-US/Vitis_Libraries/hpc/index.html).

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
