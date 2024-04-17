.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: BLAS, Library, Vitis BLAS Library, L3, level 3
   :description: Vitis BLAS library level 3 appliction programming interface benchmark.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _benchmark_l3:

=====================
L3 API Benchmark
=====================

.. toctree::
   :maxdepth: 3
   
   L3 API GEMM benchmark <L3_benchmark_gemm.rst>


Benchmark Test Overview
============================

Here are the benchmarks of the AMD Vitis™ BLAS library using the Vitis environment and comparing with the Intel Math Kernel Library. It supports hardware emulation as well as running hardware accelerators on the AMD Alveo™ U250.

1.1 Prerequisites
--------------------

1.1.1 Vitis BLAS Library
^^^^^^^^^^^^^^^^^^^^^^^^^^

- Alveo U250 installed and configured as per https://www.xilinx.com/products/boards-and-kits/alveo/u250.html#gettingStarted (when running hardware)
- XRT installed
- Vitis 2022.2 installed and configured

1.1.2 Interl MK Library
^^^^^^^^^^^^^^^^^^^^^^^^^

- Download and install MKL from https://software.intel.com/en-us/mkl/choose-download/linux.

1.2  Benchmark with CPU
-------------------------

1.2.1 Step 1:
^^^^^^^^^^^^^^^^^

Set up and build the environment using MKL scripts:

.. code-block:: bash 

    source <INTEL_MKL_INSTALL_DIR>/bin/mklvars.sh intel64


1.2.2 Step 2:
^^^^^^^^^^^^^^^^^

Run the Makefile command. For example:

.. code-block:: bash 

    cd gemm_mkl
    ./run_gemm_mkl.sh 16 float b


1.3 Building and Running the Kernel with Vitis
--------------------------------------------

Take gemm/memKernel as an example to indicate how to build the application and kernel with the command line Makefile flow.

1.3.1 Download the Code
^^^^^^^^^^^^^^^^^^^^^

These BLAS benchmarks can be downloaded from the [vitis libraries](https://github.com/Xilinx/Vitis_Libraries.git) ``main`` branch.

.. code-block:: bash 

   git clone https://github.com/Xilinx/Vitis_Libraries.git
   cd Vitis_Libraries
   git checkout main
   cd blas


1.3.2 Set Up the Environment
^^^^^^^^^^^^^^^^^^^^^^^^^

Set up and build the environment using the Vitis and XRT scripts:

.. code-block:: bash 

    source <install path>/Vitis/2022.2/settings64.sh
    source /opt/xilinx/xrt/setup.sh

K1.3.3 Build and Run the Kernel
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Run the Makefile command. For example:

.. code-block:: bash 

    make run TARGET=hw PLATFORM_REPO_PATHS=/opt/xilinx/platforms PLATFORM=xilinx_u250_xdma_201830_2


The Makefile supports various build targets including hw emulation and hardware (hw_emu, hw).

The host application can be run manually using the following pattern:

.. code-block:: bash 

    <host application> <xclbin> <argv>


For example:

.. code-block:: bash 

    build_dir.hw.xilinx_u250_xdma_201830_2/host.exe build_dir.hw.xilinx_u250_xdma_201830_2/blas.xclbin build_dir.hw.xilinx_u250_xdma_201830_2/config_info.dat

    

