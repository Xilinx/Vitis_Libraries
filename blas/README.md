# Vitis BLAS Library

Vitis BLAS Library is an open-sourced Vitis library written in C++ and released under
[Apache 2.0 license](https://www.apache.org/licenses/LICENSE-2.0)
for accelerating linear algebra functions in a variety of use cases.

The main target audience of this library is users who want to accelerate
linear algebra functions with FPGA cards.
Currently, this library offers three levels of acceleration:

* At module level is for the C++ implementation of BLAS functions. These implementations are intended to be used by HLS (High Level Synthesis) users to build FPGA logic for their applications. 
* The kernel level is for pre-defined kernels that are the C++ implementation of BLAS functions. These implementations are intended to demonstrate how FPGA kernels are defined and how L1 primitive functions can be used by any Vitis users to build their kernels for their applications. 
* The software APIs level is an implementation of BLAS on top of the XILINX runtime (XRT). It allows software developers to use Vitis BLAS library without writing any runtime functions and hardware configurations.

Check the [comprehensive HTML document](https://docs.xilinx.com/r/en-US/Vitis_Libraries/blas/index.html) for more details.


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
    source <install path>/Vitis/2023.2/settings64.sh
    source /opt/xilinx/xrt/setup.sh
```

### HLS Cases Command Line Flow

```console
cd L1/tests/case_folder/
make run TARGET=<cosim/csim/csynth/vivado_syn/vivado_impl> PLATFORM=/path/to/xilinx_u250_gen3x16_xdma_3_1_202020_1.xpfm
```

- `csim` (high level simulation),
- `csynth` (high level synthesis to RTL),
- `cosim` (cosimulation between software testbench and generated RTL),
- `vivado_syn` (synthesis by Vivado) and
- `vivado_impl` (implementation by Vivado).

### Vitis Cases Command Line Flow

#### L2

```console
cd L2/tests/vitis_case_folder

# build and run one of the following using U280 platform
make run TARGET=hw_emu PLATFORM=/path/to/xilinx_u250_gen3x16_xdma_3_1_202020_1.xpfm

# delete generated files
make cleanall
```

#### L3

```console
cd L3/tests/vitis_case_folder

# build and run one of the following using U280 platform
make run TARGET=hw_emu PLATFORM=/path/to/xilinx_u250_gen3x16_xdma_3_1_202020_1.xpfm

# delete generated files
make cleanall
```

Here, `TARGET` decides the FPGA binary type

- `hw_emu` is for hardware emulation
- `hw` is for deployment on physical card. (Compilation to hardware binary often takes hours.)

Besides ``run``, the Vitis case makefile also allows ``host`` and ``xclbin`` as build target.

## Benchmark Result

More details about the benchmarks, please kindly find them in [benchmark results](https://docs.xilinx.com/r/en-US/Vitis_Libraries/blas/benchmark.html).

## Documentations
For more details of the blas library, please refer to [blas Library Documentation](https://docs.xilinx.com/r/en-US/Vitis_Libraries/blas/index.html).


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
