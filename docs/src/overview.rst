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

.. _overview:

.. toctree::
   :hidden:

Requirements
------------

Software Platform
~~~~~~~~~~~~~~~~~

This library is designed to work with Vitis 2019.2 and later, and therefore inherits the system requirements of Vitis and XRT.

Supported operating systems are RHEL/CentOS 7.4, 7.5 and Ubuntu 16.04.4 LTS, 18.04.1 LTS.
With CentOS/RHEL 7.4 and 7.5, C++11/C++14 should be enabled via
`devtoolset-6 <https://www.softwarecollections.org/en/scls/rhscl/devtoolset-6/>`_.

PCIE Accelerator Card
~~~~~~~~~~~~~~~~~~~~~

Hardware modules and kernels are designed to work with 16nm Alveo cards. GQE kernels are best tuned for U280, and could be tailored for other devices.

* `Alveo U280 <https://www.xilinx.com/products/boards-and-kits/alveo/u280.html#gettingStarted>`_
* `Alveo U250 <https://www.xilinx.com/products/boards-and-kits/alveo/u250.html#gettingStarted>`_
* `Alveo U200 <https://www.xilinx.com/products/boards-and-kits/alveo/u200.html#gettingStarted>`_


Shell Environment
~~~~~~~~~~~~~~~~~

Setup the build environment using the Vitis and XRT scripts.



.. code-block:: shell

    source /opt/xilinx/Vitis/2019.2/settings64.sh
    source /opt/xilinx/xrt/setup.sh
    export PLATFORM_REPO_PATHS=/opt/xilinx/platforms


Setting ``PLATFORM_REPO_PATHS`` to the installation folder of platform files can enable makefiles
in this library to use ``DEVICE`` variable as a pattern.
Otherwise, full path to .xpfm file needs to be provided via ``DEVICE`` variable.

Dependency
~~~~~~~~~~

This library depends on the Vitis Utility Library, which is assumed to be placed in the
same path as this library with name ``utils``. Hence the directory is organized as follows.

.. code-block:: shell

    /cloned/path/database # This library, which contains L1, L2, etc.
    /cloned/path/utils # The Vitis Utility Library, which contains its L1.


Design Flows
------------

Recommended design flows are categorised by the target level:

* L1
* L2

The common tool and library pre-requisites that apply across all design flows are documented in the requirements section above.

L1
~~

L1 provides the basic modules could be used to build GQE kernels.

The recommend flow to evaluate and test L1 components is described as follows using Vivado HLS tool.
A top level C/C++ testbench (typically ``algorithm_name.cpp``) prepares the input data, passes them to the design under test, then performs output data post processing and validation checks.

A Makefile is used to drive this flow with available steps including ``CSIM`` (high level simulation), ``CSYNTH`` (high level synthesis from C/C++ to RTL) and ``COSIM`` (co-simulation between software testbench and generated RTL), ``VIVADO_SYN`` (synthesis by Vivado), ``VIVADO_IMPL`` (implementation by Vivado). The flow is launched from the shell by calling ``make`` with variables set as in the example below:

.. code-block:: shell

    cd L1/tests/specific_algorithm/
    make run CSIM=1 CSYNTH=0 COSIM=0 VIVADO_SYN=0 VIVADO_IMPL=0 \
    DEVICE=/path/to/xilinx_u280_xdma_201910_1.xpfm


As well as verifying functional correctness, the reports generated from this flow give an indication of logic utilization, timing performance, latency and throughput. The output files of interest can be located at the location of the test project where the path name is correlated with the algorithm. i.e. the callable function within the design under test.

L2
~~

L2 provides the Generic Query Engine (GQE) kernels.

The available flow for L2 based around the Vitis tool facilitates the generation and packaging of GQE kernels along with the required host application for configuration and control. In addition to supporting FPGA platform targets, emulation options are available for preliminary investigations, or where dedicated access to a hardware platform may not be available. Two emulation options are available, the software emulation which performs a high level simulation of the design, and the hardware emulation which performs a cycle-accurate simulation of the generated RTL for the kernel. This flow is makefile-driven from the console where the target is selected by a command line parameter as in the example below:

.. code-block:: shell

    cd L2/tests/specific_GQE_kernel

    # build and run one of the following using U280 platform
    #  * software emulation,
    #  * hardware emulation,
    #  * actual deployment on physical platform
    
    make run TARGET=sw_emu DEVICE=/path/to/xilinx_u280_xdma_201910_1.xpfm
    make run TARGET=hw_emu DEVICE=/path/to/xilinx_u280_xdma_201910_1.xpfm
    make run TARGET=hw DEVICE=/path/to/xilinx_u280_xdma_201910_1.xpfm

    # delete all xclbin and host binary
    make cleanall


The outputs of this flow are packaged kernel binaries (xclbin files) that can be downloaded to the FPGA platform, and host executables to configure and coordinate data transfers. The output files of interest can be located where the file names are correlated with the specific query.

This flow can be used to verify functional correctness in hardware and meature actual performance.


License
-------

Licensed using the `Apache 2.0 license <https://www.apache.org/licenses/LICENSE-2.0>`_.

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

Trademark Notice
----------------

    Xilinx, the Xilinx logo, Artix, ISE, Kintex, Spartan, Virtex, Zynq, and
    other designated brands included herein are trademarks of Xilinx in the
    United States and other countries.  All other trademarks are the property
    of their respective owners.

