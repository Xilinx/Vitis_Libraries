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

*****************************
Vitis Data Analytics Library
*****************************

Vitis Data Analytics Library is an open-sourced Vitis library written in C++ for accelerating data analytics applications in a variety of use cases.

It now covers L1 and L2 level primitives. It provides optimized hardware implementation of most common data analytics algorithms.

Since all the primitive code is developed in HLS C++ with the permissive Apache 2.0 license,
advanced users can easily tailor, optimize or assemble property logic.


Shell Environment
==================

Setup the build environment using the Vitis and XRT scripts, and set the PLATFORM_REPO_PATHS to installation folder of platform files.

.. code-block:: bash

    source <install path>/Vitis/2019.2/settings64.sh
    source /opt/xilinx/xrt/setup.sh
    export PLATFORM_REPO_PATHS=/opt/xilinx/platforms

Design Flows
============

The common tool and library pre-requisites that apply across all design flows are documented in the requirements section above.

Recommended design flows are decribed as follows:

The recommend flow to evaluate and test L1 components is described as follows using Vivado HLS tool.
A top level C/C++ testbench (typically `algorithm_name.cpp`) prepares the input data, passes them to the design under test, then performs any output data post processing and validation checks.

A Makefile is used to drive this flow with available steps including `CSIM` (high level simulation), `CSYNTH` (high level synthesis to RTL) and `COSIM` (cosimulation between software testbench and generated RTL), `VIVADO_SYN` (sy
nthesis by Vivado), `VIVADO_IMPL` (implementation by Vivado). The flow is launched from the shell by calling `make` with variables set as in the example below:


.. code-block:: bash

   . /opt/xilinx/xrt/setup.sh
   export PLATFORM_REPO_PATHS=/opt/xilinx/platforms
   cd L1/tests/specific_algorithm/
   # Only run C++ simulation on U250 card
   make run CSIM=1 CSYNTH=0 COSIM=0 VIVADO_SYN=0 VIVADO_IMPL=0 DEVICE=u250_xdma_201830_1


As well as verifying functional correctness, the reports generated from this flow give an indication of logic utilization, timing performance, latency and throughput. The output files of interest can be located at the location of
 the test project where the path name is correlated with the algorithm. i.e. the callable function within the design under test.

To run the Vitis projects for benchmark evaluation and test, you may need the example below:


.. code-block:: bash

    ./opt/xilinx/xrt/setup.sh
    export PLATFORM_REPO_PATHS=/opt/xilinx/platforms
    d L1/benchmarks/specific_algorithm/
    # Run software emulation for correctness test
    make run TARGET=sw_emu DEIVCE=u250_xdma_201830_1
    # Run hardware emulation for cycle-accurate simulation with the RTL-model
    make run TARGET=hw_emu DEIVCE=u250_xdma_201830_1
    # Run hardware to generate the desired xclbin binary
    make run TARGET=hw DEIVCE=u250_xdma_201830_1
    # Delete xclbin and host excutable program
    make cleanall


.. toctree::
   :caption: Library Overview
   :maxdepth: 1

   overview.rst
   release.rst

.. toctree::
   :caption: L1 User Guide
   :maxdepth: 2

   guide_L1/hw_guide.rst


.. toctree::
   :caption: L2 User Guide
   :maxdepth: 2

   guide_L2/hw_guide.rst


.. toctree::
   :caption: Benchmark Result
   :maxdepth: 1

   benchmark/result.rst

Index
-----

* :ref:`genindex`
