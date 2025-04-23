.. 
   .. Copyright © 2020–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: libgraphL3.so, getting started, setup, environment, dynamic library
   :description: Getting started with Graph library.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

********************************
Getting Started
********************************

To use Graph L3, the hardware requirements should be met and the shared library (**libgraphL3.so**) should be built and linked in the user application.  

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
BASH version setup scripts are provided. You should select a proper setup script depending on the shell version.

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

To build **libgraphL3.so**, follow the following steps:

.. code-block:: sh

	cd xf_graph/L3/lib
	./build_so.sh --target hw_emu/hw
  
Choose the target type and run the script. After the build is complete, **libgraphL3.so** should be available in *Vitis_Libraries/graph/L3/lib*.


Run the testcases
############################

There are many testcases provided for L3 APIs. To run testacses, follow the following steps:

.. code-block:: sh

	cd xf_graph/L3/tests/cosineSimilaritySSDenseInt/
    source /LOCAL PATH to VITIS/settings64.sh
    export PLATFORM=/LOCAL PATH to TARGET PLATFORM/PLATFORM.xpfm
	make run TARGET=hw_emu/hw 
  
It automatically builds host.exe and xclbin file, then run the testcase of single source integer cosine similarity in dense graph. Other testcases can be performed by the same way.

