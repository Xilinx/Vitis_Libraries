# Vitis Quantitative Finance Library

The Vitis Quantitative Finance Library is an fundamental library aimed at providing a comprehensive FPGA acceleration library for quantitative finance.
It is a free/open-source for a variety of real use case, such as modeling, trading, evaluation, risk-management and so on.

The Vitis Quantitative Finance Library provides comprehensive tools from the bottom up for quantitative finance. It includes the lowest level modules and functions, the pre-defined middle kernel level, and the third level as pure software APIs working with pre-defined hardware overlays.

* At the lowest level (L1), it offers useful statistical functions, numerical methods and linear algebra functions to support practical user to implement advanced modeling, such as RNG, Monte Carlo, SVD, specialist matrix solvers and so on.

* In the middle level (L2), pricing engines kernel are provided to evaluate common finance derivatives, such as equity products, interest-rate products, FX products, and credit products.

* The software API level (L3) wraps the details of offloading acceleration with pre-built binary (overlay) and allow users to accelerate supported pricing tasks on Alveo cards without hardware development.

Check the [comprehensive HTML documentation](https://pages.gitenterprise.xilinx.com/FaaSApps/xf_fintech/index.html) for more details.

## Library Contents

| Library Function | Description | Layer |
|------------------|-------------|-------|
| Mersenne Twister RNG (MT19937) | Random number generator | L1 |
| Mersenne Twister RNG (MT2203) | Random number generator | L1 |
| RNG (LUT-SR)         | Random number generator.  | L1    |
| RNG (Gaussian)       | Random number generator.  | L1    |
| Box-Muller Transform | Produces normal distribution from a uniform one. | L1 |
| 1-dimensional  Sobol  | Quasi-random number generator.      | L1    |
| Multi-dimensional  Sobol  | Quasi-random number generator.      | L1    |
| Inverse Cumulative Normal Distribution | | L1 |
| Brownian Bridge Transform |  | L1 |
| Jacobi SVD | Singular Value Decomposition using the Jacobi method | L1 |
| Monte Carlo (discretization quantization)        |                           | L1    |
| Tridiagonal Solver | Solver for tridiagonal systems of equations using PCR | L1 |
| Pentadiagonal Solver | Solver for pentadiagonal systems of equations using PCR | L1 |
| 1-dimentional Stochastic Process | derived by RNGs | L1 |
| Ornstein Uhlenbeck Process | A simple stochastic process derived by RNGs | L1 |
| Mesher | Discretization | L1 |
| Monte-Carlo Heston         | Monte-Carlo Heston (model only). | L2    |
| Monte-Carlo European Heston | Monte-Carlo simulation of European-style options using Heston model | L2 |
| Monte-Carlo European Heston Greeks | Measure sensitivity of derivative values | L2 |
| Monte-Carlo Black-Scholes European | Monte-Carlo simulation of European-style options using the Black-Scholes model | L2 |
| Monte-Carlo Multi-Asset European Heston | Monte-Carlo simulation of European-style options for multiple underlying asset | L2 |
| Monte-Carlo Black-Scholes American | Monte-Carlo simulation of American-style options using the Black-Scholes model | L2 |
| Monte-Carlo Black-Scholes Digital  | Monte-Carlo simulation of digital (all-or-nothing) option using the Black-Scholes model | L2 |
| Monte-Carlo Black-Scholes Asian    | Monte-Carlo simulation of Asian-style options using the Black-Scholes model | L2 |
| Monte-Carlo Black-Scholes Barrier  | Monte-Carlo simulation of barrier options using the Black-Scholes model | L2 |
| Monte-Carlo Cliquet | Monte-Carlo simulation of cliquet option | L2 |
| Markov Chain of Monte-Carlo | For sampling | L2 |
| Closed Form Black Scholes Merton | Derivate investment instruments | L2 |
| Closed Form Heston | Add stochastic volatility | L2 |
| Closed Form Merton 76 | Add random jump to Black Scholes model | L2 |
| Garman Kohlhagen | Model for foreign exchange based on Black Scholes Merton | L2 |
| Quanto | | L2 |
| Cox-Ross-Rubinstein Binomial Tree | Numerical assumptions in black-Scholes model | L2 |
| Tree Bermudan Swaption | | L2 |
| CPI CapFloor | Use linear interpolation | L2 |
| Inflation CapFloor | Year-on-year inflation cap/floor | L2 |
| Zero Coupon Bond | | L2 |
| Monte-Carlo Hull-White | Monte-Carlo simulation of cap/floor using Hull-White model | L2 |
| Finite-Difference Heston | Solution of the Heston model using an ADI finite-difference solver | L2 |
| Finite-Difference Hull-White Bermudan Swaption Pricing | Estimate Bermudan Swaption based on FDM | L2 |
| Finite-Difference G2 Bermudan Swaption Pricing | Estimate Bermudan Swaption based on FDM | L2 |
| Host API | Unified Host-callable API for the simulation engines | L3 |

## Requirements

### Software Platform

This library is designed to work with Vitis 2019.2 and later, and therefore inherits the system requirements of Vitis and XRT.

Supported operating systems are RHEL/CentOS 7.4, 7.5 and Ubuntu 16.04.4 LTS, 18.04.1 LTS.
With CentOS/RHEL 7.4 and 7.5, C++11/C++14 should be enabled via
[devtoolset-6](https://www.softwarecollections.org/en/scls/rhscl/devtoolset-6/).

### Dependencies

The Vitis Quantitative Finance Library also needs the XF RNG and XF BLAS libraries; these should be cloned or unpacked
at the same directory level as the Vitis Quantitative Finance library.

### PCIE Accelerator Card

Hardware modules and kernels are designed to work with Alveo cards. Specific requirements are noted against each kernel or demonstration. Hardware builds for Alveo board targets require package installs as per:
* [Alveo U200](https://www.xilinx.com/products/boards-and-kits/alveo/u200.html#gettingStarted)
* [Alveo U250](https://www.xilinx.com/products/boards-and-kits/alveo/u250.html#gettingStarted)

### Shell Environment

Setup the build environment using the Vitis and XRT scripts:

```console
    $ source <install path>/Vitis/2019.2/settings64.sh
    $ source /opt/xilinx/xrt/setup.sh
```

## Design Flows

Recommended design flows are categorised by the target level:

* L1
* L2
* L3

The common tool and library prerequisites that apply across all design flows are documented in the requirements section above.

### L1

L1 provides the low-level primitives used to build kernels.

The recommend flow to evaluate and test L1 components uses the Vivado HLS tool. A top level C/C++ testbench (typically `main.cpp`) prepares the input data, passes this to the design under test (typically `dut.cpp` which makes the L1 level library calls) then performs any output data post processing and validation checks.

A Makefile is used to drive this flow with available steps including `CSIM` (high level simulation), `CSYNTH` (high level synthesis to RTL) and `COSIM` (cosimulation between software testbench and generated RTL). The flow is launched from the shell by calling `make` with variables set as in the example below:

```console
$ . /opt/xilinx/xrt/setup.sh
$ export PLATFORM_REPO_PATHS=/opt/xilinx/platforms
$ cd L1/tests/ICD/
$ make CSIM=1 CSYNTH=0 COSIM=0 check # Only run C++ simulation
```

With no make variables set (i.e. `make check`), the Makefile will high level simulate, synthesize to RTL and cosimulate.

As well as verifying functional correctness, the reports generated from this flow give an indication of logic utilisation, timing performance, latency and throughput. The output files of interest can be located at the location examples as below where the file names are correlated with the source code. i.e. the callable functions within the design under test.

    Simulation Log: L1/tests/ICD/prj/sol/csim/report/ICN_top_csim.log
    Synthesis Report: L1/tests/ICD/prj/sol/syn/report/ICN_top_csynth.rpt

### L2

L2 provides the pricing engine APIs presented as kernels.

There are two available flows for evaluating and testing L2 components. The first of these is the Vivado HLS testbench Makefile-driven flow as previously described for L1 components:

```console
$ cd L2/tests/McEuropeanEngine/
$ make CSIM=1 CSYNTH=0 COSIM=0 check # Only run C++ simulation
```

Again, the default is to run high level simulation, synthesize to RTL and cosimulate.

A second available flow for L2 based around the SDAccel tool facilitates the generation and packaging of pricing engine kernels along with the required host application for configuration and control. In addition to supporting FPGA platform targets, emulation options are available for preliminary investigations or where dedicated access to a hardware platform may not be available. Two emulation options are available, software emulation which performs a high level simulation of the pricing engine and hardware emulation which performs a cycle-accurate simulation of the generated RTL for the kernel. This flow is makefile driven from the console where the target is selected as a command line parameter as in the examples below:

```console
$ cd L2/demos/MCEuropeanEngine

# run one of the following, software emulation, hardware emulation or actual deployment on physical platform
$ make run_sw_emu
$ make run_hw_emu
$ make run_hw

# after a hardware system build has been completed, execution can be repeated by running the host executable and passing it the xclbin as an argument
$ ./bin/test.exe -mode fpga -xclbin xclbin/kernel_mc_u250_hw.xclbin
```

The outputs of this flow are packaged kernel binaries (xclbin files) that can be downloaded to the FPGA platform and host executables to configure and co-ordinate data transfers. The output files of interest can be located at the locations examples as below where the file names are correlated with the source code

    Host Executable: L2/demos/MCEuropeanEngine/bin/test.exe
    Kernel Packaged Binary: L2/demos/MCEuropeanEngine/xclbin/kernel_mc_u250_hw.xclbin

This flow can be used to verify functional correctness in hardware and enable real world performance to be measured.

### L3

L3 provides the high level software APIs to deploy and run pricing engine kernels whilst abstracting the low level details of data transfer, kernel related resources configuration, and task scheduling.

The flow for L3 is the only one where access to an FPGA platform is required.

A prerequisite of this flow is that the packaged pricing engine kernel binaries (xclbin files) for the target FPGA platform target have been made available for download or have been custom built using the L2 flow described above.

This flow is makefile driven from the console to initially generate a shared object (`L3/src/output/libxilinxfintech.so`).

```console
$ cd L3/src
$ source env.sh
$ make
```

The shared object file is written to the location examples as below:

    Library: L3/src/output/libxilinxfintech.so

User applications can subsequently be built against this library as in the example provided:

```console
$ cd L3/examples/MonteCarlo
$ make all
$ cd output

# manual step to copy or create symlinks to xclbin files in current directory

$ ./mc_example
```

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


