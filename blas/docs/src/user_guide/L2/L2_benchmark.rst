.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: BLAS, Library, Vitis BLAS Library, L2, level 2
   :description: Vitis BLAS library level 2 appliction programming interface benchmark.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _benchmark_l2:

=====================
L2 API Benchmark
=====================

.. toctree::
   :maxdepth: 3
   
   L2 GEMM benchmark <L2_benchmark_gemm.rst>
   L2 GEMV benchmark <L2_benchmark_gemv.rst>


Benchmark Test Overview
============================

Here are benchmarks of the AMD Vitis™ BLAS library using the Vitis environment. It supports software and hardware emulation as well as running hardware accelerators on the AMD Alveo™ U250.

1.1 Prerequisites
----------------------

1.1.1 Vitis BLAS Library
^^^^^^^^^^^^^^^^^^^^^^^^^^^

- Alveo U250 installed and configured as per https://www.xilinx.com/products/boards-and-kits/alveo/u250.html#gettingStarted (when running hardware)
- XRT installed
- Vitis 2022.2 installed and configured

1.2 Building
----------------

Take gemm_4CU as an example to indicate how to build the application and kernel with the command line Makefile flow.

1.2.1 Download Code
^^^^^^^^^^^^^^^^^^^^^

These BLAS benchmarks can be downloaded from the [vitis libraries](https://github.com/Xilinx/Vitis_Libraries.git) ``main`` branch.

.. code-block:: bash 

   git clone https://github.com/Xilinx/Vitis_Libraries.git
   cd Vitis_Libraries
   git checkout main
   cd blas

   
1.2.2 Set Up the Environment
^^^^^^^^^^^^^^^^^^^^^^^^^^

Set up and build the environment using the Vitis and XRT scripts:

.. code-block:: bash 

    source <install path>/Vitis/2022.2/settings64.sh
    source /opt/xilinx/xrt/setup.sh


1.2.3 Build and Run the Kernel
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Run the Makefile command. For example:

.. code-block:: bash 

    make run TARGET=hw PLATFORM_REPO_PATHS=/opt/xilinx/platforms PLATFORM=xilinx_u250_xdma_201830_2

    
The Makefile supports various build target including software emulation, hw emulation, and hardware (sw_emu, hw_emu, hw).

The host application can be run manually using the following pattern:

.. code-block:: bash 

    <host application> <xclbin> <argv>

    
For example:

.. code-block:: bash 

    build_dir.hw.xilinx_u250_xdma_201830_2/host.exe build_dir.hw.xilinx_u250_xdma_201830_2/blas.xclbin 64 64 64

