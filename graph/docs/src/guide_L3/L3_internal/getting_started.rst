.. 
   Copyright 2020 Xilinx, Inc.
  
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
  
       http://www.apache.org/licenses/LICENSE-2.0
  
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

.. meta::
   :keywords: libgraphL3.so, getting started, setup, environment, dynamic library
   :description: Getting started with Graph library.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

********************************
Getting Started
********************************

In order to use Graph L3, the sofware and hardware requirements should be met and the shared library (**libgraphL3.so**) should be built and linked in the users application.  

Software Requirements
#####################
* CentOS/RHEL 7.8 and Ubuntu 16.04 LTS
* `Xilinx RunTime (XRT) <https://github.com/Xilinx/XRT>`_ 2022.1
* `Xilinx FPGA Resource Manager (XRM) <https://github.com/Xilinx/XRM>`_ 2022.1


Hardware Requirements
#####################
* `Alveo U50 <https://www.xilinx.com/products/boards-and-kits/alveo/u50.html>`_
* `Alveo U55C <https://www.xilinx.com/applications/data-center/high-performance-computing/u55c.html>`_


Environment Setup
#################
BASH version setup scripts are provided. Users should select their proper setup script depending on their shell version.

BASH:

.. code-block:: sh

	source /LOCAL PATH to XRT/setup.sh
	source /LOCAL PATH to XRM/setup.sh
	source /LOCAL PATH to XRM/start_xrmd.sh

These scripts set up the following environment variables:

* XILINX_XRT
	* This points to */opt/xilinx/xrt/*

* XILINX_XRM
	* This points to */opt/xilinx/xrm/*


Build the dynamic library
############################

In order to build **libgraphL3.so**, please follow the following steps:

.. code-block:: sh

	cd xf_graph/L3/lib
	./build_so.sh --target sw_emu/hw_emu/hw
  
Choose the target type and run the script. After the build is complete, **libgraphL3.so** should be available in *Vitis_Libraries/graph/L3/lib*


Run the testcases
############################

There are many testcases provided for L3 APIs. In order to run testacses, please follow the following steps:

.. code-block:: sh

	cd xf_graph/L3/tests/cosineSimilaritySSDenseInt/
    source /LOCAL PATH to VITIS/settings64.sh
    export PLATFORM=/LOCAL PATH to TARGET PLATFORM/PLATFORM.xpfm
	make run TARGET=sw_emu/hw_emu/hw 
  
It will automatically build host.exe and xclbin file, then run the testcase of single sourse integer cosine similarity in dense graph. Other testcases could be performed by the same way.

