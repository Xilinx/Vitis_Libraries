.. 
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

**********************************
Vitis Quantitative Finance Library
**********************************

The Vitis Quantitative Finance Library is a Vitis Library aimed at providing a comprehensive FPGA acceleration library for quantitative finance. 
It is an open-source library that can be used in a variety of financial applications, such as modeling, trading, evaluation and risk management.

The Vitis Quantitative Finance Library provides extensive APIs at three levels of abstraction:

* L1, the basic functions heavily used in higher level implementations. It includes statistical functions such as Random Number Generation (RNG), numerical methods, e.g., Monte Carlo Simulation, and linear algebra functions such as Singular Value Decomposition (SVD), and tridiagonal and pentadiagonal matrix solvers.

* L2, the APIs provided at the level of pricing engines. Various pricing engines are provided to evaluate different financial derivatives, including equity products, interest-rate products, foreign exchange (FX) products, and credit products. At this level, each pricing engine API can be seen as a kernel. The customers may write their own CPU code to call different pricing engines under the framework of OpenCL.  

* L3, the software level APIs. APIs of this level hide the details of data transfer, kernel related resources configuration, and task scheduling in OpenCL. Software application programmers may quickly use L3 high-level APIs to run various pricing options without touching the dependency of OpenCL tasks and hardware configurations. 
  
Library Contents
================

+------------------------------------------------------------------------------------------------+---------------------------+-------+
| Library Function                                                                               | Description               | Layer |
+================================================================================================+===========================+=======+
| :ref:`Mersenne Twister RNG MT19937 <cid-xf::fintech::mt19937>`                                 | Random number generator   | L1    |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
| :ref:`Mersenne Twister RNG MT2203 <cid-xf::fintech::mt2203>`                                   | Random number generator   | L1    |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
| RNG (LUT-SR)                                                                                   | Random number generator   | L1    |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
| RNG (Gaussian)                                                                                 | Random number generator   | L1    |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                | Produces a normal         | L1    |
| :ref:`Box-Muller Transform <cid-xf::fintech::mt19937boxmullernormalrng>`                       | distribution from a       |       |
|                                                                                                | uniform one               |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                | Quasi-random number       | L1    |
|                                                                                                | generator                 |       |
| :ref:`1-D Sobol <cid-xf::fintech::sobolrsg1d>`                                                 |                           |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                | Quasi-random number       | L1    |
|                                                                                                | generator                 |       |
| :ref:`Multi-dimensional Sobol <cid-xf::fintech::sobolrsg>`                                     |                           |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
| :ref:`Inverse Cumulative Normal Distribution <cid-xf::fintech::inversecumulativenormalppnd7>`  |                           | L1    |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
| :ref:`Brownian Bridge Transform <cid-xf::fintech::brownianbridge>`                             |                           | L1    |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                | Singular Value            | L1    |
| :ref:`Jacobi SVD <cid-xf::fintech::svd>`                                                       | Decomposition using the   |       |
|                                                                                                | Jacobi method             |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                | Solver for tridiagonal    | L1    |
| :ref:`Tridiagonal Solver <cid-xf::fintech::trsvCore>`                                          | systems of equations      |       |
|                                                                                                | using PCR                 |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                | Solver for pentadiagonal  | L1    |
| :ref:`Pentadiagonal Solver <cid-xf::fintech::pentadiagCr>`                                     | systems of equations      |       |
|                                                                                                | using PCR                 |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                | 1-dimentional stochastic  |       |
| :ref:`1-D Stochastic Process <cid-xf::fintech::stochasticprocess1d>`                           | process derived by RNG    | L1    |
|                                                                                                |                           |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                | A simple stochastic       |       |
| :ref:`Ornstein Uhlenbeck Process <cid-xf::fintech::ornsteinuhlenbeckprocess>`                  | process                   | L1    |
|                                                                                                |                           |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                | Discretization for finite |       |
| :ref:`Mesher <cid-xf::fintech::fdm1dmesher>`                                                       | difference method         | L1    |
|                                                                                                |                           |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                | Monte-Carlo Heston (model | L2    |
|                                                                                                | only)                     |       | 
| :ref:`Monte-Carlo Heston <cid-xf::fintech::hestonmodel>`                                       |                           |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                | Monte-Carlo simulation of | L2    |
|                                                                                                | European-style options    |       | 
| :ref:`Monte-Carlo European <cid-xf::fintech::mceuropeanengine>`                                |                           |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                | Monte-Carlo simulation of | L2    |
|                                                                                                | European-style options    |       | 
| :ref:`Monte-Carlo European Heston <cid-xf::fintech::mceuropeanhestonengine>`                   | using Heston model        |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                | Monte-Carlo simulation of | L2    |
|                                                                                                | European-style options    |       |
|                                                                                                | using the Black-Scholes   |       | 
| :ref:`Monte-Carlo Black-Scholes European <cid-xf::fintech::mceuropeanengine>`                  | model                     |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                | Monte-Carlo simulation of | L2    |
|                                                                                                | European-style options    |       |
|                                                                                                | for multiple underlying   |       | 
| :ref:`Monte-Carlo Multi-Asset European Heston <cid-xf::fintech::mceuropeanengine>`             | asset                     |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                | Monte-Carlo simulation of | L2    |
|                                                                                                | American-style options    |       |
|                                                                                                | using the Black-Scholes   |       |
| :ref:`Monte-Carlo Black-Scholes American <cid-xf::fintech::mcamericanengine>`                  | model                     |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                | Measure sensitivity of    | L2    |
|                                                                                                | derivative values         |       | 
| :ref:`Monte-Carlo European Heston Greeks <cid-xf::fintech::mceuropeanhestongreeksengine>`      |                           |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                | Monte-Carlo simulation of | L2    |
|                                                                                                | digital (all-or-nothing)  |       |
|                                                                                                | option using the          |       |
| :ref:`Monte-Carlo Black-Scholes Digital <cid-xf::fintech::mcdigitalengine>`                    | Black-Scholes model       |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                | Monte-Carlo simulation of | L2    |
|                                                                                                | Asian-style options using |       |
| :ref:`Monte-Carlo Black-Scholes Asian <cid-xf::fintech::mcasianarithmeticapengine>`            | the Black-Scholes model   |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                | Monte-Carlo simulation of | L2    |
|                                                                                                | barrier options using the |       |
|                                                                                                | Black-Scholes model       |       |
| :ref:`Monte-Carlo Black-Scholes Barrier <cid-xf::fintech::mcbarrierengine>`                    |                           |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                | Monte-Carlo simulation of | L2    |
|                                                                                                | cliquet option            |       | 
| :ref:`Monte-Carlo Cliquet <cid-xf::fintech::mccliquetengine>`                                  |                           |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                | For sampling              | L2    |
|                                                                                                |                           |       | 
| :ref:`Markov Chain of Monte-Carlo <cid-xf::fintech::mcmccore>`                                 |                           |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                | Monte-Carlo simulation of | L2    |
|                                                                                                | cap/floor using Hull-White|       |
| :ref:`Monte-Carlo Hull-White <cid-xf::fintech::mchullwhitecapfloorengine>`                     | model                     |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                | Solution of the Heston    | L2    |
|                                                                                                | model using an ADI        |       |
| :ref:`Finite-Difference Heston <cid-xf::fintech::fddouglas>`                                   | finite-difference solver  |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                | Derivate investment       | L2    |
|                                                                                                | instruments               |       | 
| :ref:`Closed Form Black Scholes Merton <cid-xf::fintech::cfbsmengine>`                         |                           |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                | Add stochastic volatility | L2    |
|                                                                                                |                           |       |
| :ref:`Closed Form Heston <cid-xf::fintech::hcfengine>`                                         |                           |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                | Add random jump to Black  | L2    |
|                                                                                                | Scholes model             |       |
| :ref:`Closed Form Merton 76 <cid-xf::fintech::m76engine>`                                      |                           |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                | Model for foreign exchange| L2    |
|                                                                                                | based on Black Scholes    |       |
| :ref:`Garman Kohlhagen <cid-xf::fintech::m76engine>`                                           | Merton                    |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                |                           | L2    |
| :ref:`Quanto <cid-xf::fintech::m76engine>`                                                     |                           |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                | Numerical assumptions in  | L2    |
|                                                                                                | black-Scholes model       |       |
| :ref:`Cox-Ross-Rubinstein Binomial Tree <cid-xf::fintech::binomialtreeengine>`                 |                           |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                | Estimate Bermudan Swaption| L2    |
| :ref:`Finite-Difference Hull-White Bermudan Swaption Pricing`                                  | based on FDM              |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                | Estimate Bermudan Swaption| L2    |
| :ref:`Finite-Difference G2 Bermudan Swaption Pricing`                                          | based on FDM              |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                |                           | L2    |
| :ref:`Tree Bermudan Swaption`                                                                  |                           |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                | Use linear interpolation  | L2    |
| :ref:`CPI CapFloor`                                                                            |                           |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                | Year-on-year inflation    | L2    |
| :ref:`Inflation CapFloor`                                                                      | cap/floor                 |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
|                                                                                                |                           | L2    |
| :ref:`Zero Coupon Bond`                                                                        |                           |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+
| Host API                                                                                       | Host-callable API for the | L3    |
|                                                                                                | simulation                |       |
|                                                                                                |                           |       |
+------------------------------------------------------------------------------------------------+---------------------------+-------+

Shell Environment
=================

Setup the build environment using the Vitis and XRT scripts::

    $ source <install path>/Vitis/2019.2/settings64.sh
    $ source /opt/xilinx/xrt/setup.sh

Design Flows
============

Recommended design flows are categorised by the target level:

* L1
* L2
* L3

The common tool and library prerequisites that apply across all design flows are documented in the requirements section.

L1
--

L1 provides the low-level primitives used to build kernels.

The recommend flow to evaluate and test L1 components uses the Vivado HLS tool. A top level C/C++ testbench (typically ``main.cpp``) prepares the input data, passes this to the design under test (typically ``dut.cpp`` which makes the L1 level library calls) then performs any output data post-processing and validation checks.

A Makefile is used to drive this flow with available steps including ``CSIM`` (high level simulation), ``CSYNTH`` (high level synthesis to RTL) and ``COSIM`` (cosimulation between software testbench and generated RTL). The flow is launched from the shell by calling ``make`` with variables set as in the example below::

   $ . /opt/xilinx/xrt/setup.sh
   $ export PLATFORM_REPO_PATHS=/opt/xilinx/platforms
   $ cd L1/tests/ICD/
   $ make CSIM=1 CSYNTH=0 COSIM=0 DEVICE=xilinx_u250_xdma_201830_2 check # Only run C++ simulation

As well as verifying functional correctness, the reports generated from this flow give an indication of logic utilisation, timing performance, latency and throughput. The output files of interest can be located at the example locations as shown below where the file names are correlated with the source code, i.e., the callable functions within the design under test.

::

   Simulation Log: L1/tests/ICD/prj/sol/csim/report/ICN_top_csim.log
   Synthesis Report: L1/tests/ICD/prj/sol/syn/report/ICN_top_csynth.rpt

L2
--

L2 provides the pricing engine APIs presented as kernels.

The available flow for L2 based around the SDAccel tool facilitates the generation and packaging of pricing engine kernels along with the required host application for configuration and control. In addition to supporting FPGA platform targets, emulation options are available for preliminary investigations or where dedicated access to a hardware platform may not be available. Two emulation options are available, software emulation which performs a high level simulation of the pricing engine and hardware emulation which performs a cycle accurate simulation of the generated RTL for the kernel. This flow is makefile driven from the console where the target is selected as a command line parameter as in the examples below::


   $ cd L2/tests/McEuropeanEngine
   
   # run one of the following, software emulation, hardware    emulation or actual deployment on physical platform
   $ make run_sw_emu DEVICE=xilinx_u250_xdma_201830_2
   $ make run_hw_emu DEVICE=xilinx_u250_xdma_201830_2
   $ make run_hw DEVICE=xilinx_u250_xdma_201830_2
   
   # after a hardware system build has been completed, execution  can be repeated by running the host executable and passing it   the xclbin as an argument
   $ bin_xilinx_u250_xdma_201830_2/test.exe -mode fpga -xclbin xclbin_xilinx_u250_xdma_201830_2_sw_emu/kernel_mcae.xclbin


The outputs of this flow are packaged kernel binaries (xclbin files) that can be downloaded to the FPGA platform, and host executables to configure and co-ordinate data transfers. The output files of interest can be located at the example locations as shown below where the file names are correlated with the source code.

::

    Host Executable: L2/tests/McEuropeanEngine/bin_xilinx_u250_xdma_201830_2/test.exe
    Kernel Packaged Binary: L2/tests/McEuropeanEngine/xclbin_xilinx_u250_xdma_201830_2_sw_emu/kernel_mcae.xclbin

This flow can be used to verify functional correctness in hardware and enable real world performance to be measured.

L3
--

L3 provides the high level software APIs to deploy and run pricing engine kernels whilst abstracting the low level details of data transfer, kernel related resources configuration, and task scheduling.

The flow for L3 is the only one where access to an FPGA platform is required.

A prerequisite of this flow is that the packaged pricing engine kernel binaries (xclbin files) for the target FPGA platform target have been made available for download or have been custom built using the L2 flow described above.

This flow is makefile driven from the console to initially generate a shared object (``L3/src/output/libxilinxfintech.so``)::

   $ cd L3/src
   $ source env.sh
   $ make


The shared object file is written to the example location as shown below::

    Library: L3/src/output/libxilinxfintech.so

User applications can subsequently be built against this library as in the example provided::

   $ cd L3/examples/MonteCarlo
   $ make all
   $ cd output

   # manual step to copy or create symlinks to xclbin files in current directory

   $ ./mc_example


.. toctree::
   :maxdepth: 1


.. toctree::
   :caption: Library Overview 
   :maxdepth: 1

   overview.rst
   rel.rst


.. toctree::
   :caption: User Guide
   :maxdepth: 2

   models_and_methods.rst
   guide_L1/L1.rst
   guide_L2/L2.rst
   guide_L3/L3.rst


.. toctree::
   :caption: Benchmark Result
   :maxdepth: 1

   benchmark/benchmark.rst
   



