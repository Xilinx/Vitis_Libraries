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

Hardware modules and kernels are designed to work with all Alveo cards.

* `Alveo U280 <https://www.xilinx.com/products/boards-and-kits/alveo/u280.html#gettingStarted>`_
* `Alveo U250 <https://www.xilinx.com/products/boards-and-kits/alveo/u250.html#gettingStarted>`_
* `Alveo U200 <https://www.xilinx.com/products/boards-and-kits/alveo/u200.html#gettingStarted>`_


Shell Environment
~~~~~~~~~~~~~~~~~

Setup the build environment using the Vitis and XRT scripts.

.. ref-code-block:: bash
	:class: overview-code-block

        source /opt/xilinx/Vitis/2019.2/settings64.sh
        export PLATFORM_REPO_PATHS=/opt/xilinx/platforms

Setting ``PLATFORM_REPO_PATHS`` to the installation folder of platform files can enable makefiles
in this library to use ``DEVICE`` variable as a pattern.
Otherwise, full path to .xpfm file needs to be provided via ``DEVICE`` variable.


Design Flows
------------

The common tool and library pre-requisites that apply across all design flows are documented in the requirements section above.

Recommended design flow is shown as follows:

L1 provides the modules to work distribution and result collection in different algorithms, manipulate streams: including combination, duplication, synchronization, and shuffle, updates URAM array in tighter initiation internal (II).

The recommend flow to evaluate and test L1 components is described as follows using Vivado HLS tool.
A top level C/C++ testbench (typically ``algorithm_name.cpp```) prepares the input data, passes them to the design under test, then performs any output data post processing and validation checks.

A Makefile is used to drive this flow with available steps including ``CSIM`` (high level simulation),
``CSYNTH`` (high level synthesis to RTL) and ``COSIM`` (cosimulation between software testbench and generated RTL),
``VIVADO_SYN`` (synthesis by Vivado), ``VIVADO_IMPL`` (implementation by Vivado).
The flow is launched from the shell by calling ``make`` with variables set as in the example below:

.. ref-code-block:: bash
	:class: overview-code-block
        
        cd L1/tests/specific_algorithm/
        make run CSIM=1 CSYNTH=0 COSIM=0 VIVADO_SYN=0 VIVADO_IMPL=0 \
                 DEVICE=/path/to/xilinx_u200_xdma_201830_2.xpfm

To enable more than C++ simulation, just switch other steps to `1` in `make` command line.

As well as verifying functional correctness, the reports generated from this flow give an indication of logic utilization, timing performance, latency and throughput. The output files of interest can be located at the location of the test project where the path name is "test.prj".


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

