.. GenomicsLib_Docs documentation master file, created by
   sphinx-quickstart on Thu Jan 13 14:04:09 2022.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

.. meta::
   :keywords: Vitis, Library, Genomics, Smithwaterman, Xilinx, PairHMM, SMEM.
   :description: This page provides benchmarking results of various Vitis Genomics Applications. Results include throughput and FPGA resources.

==========
Benchmark
==========

Datasets
````````
Benchmark evaluation of genomics algorithm performance is with random data

Genomics Performance
```````````````````````

The following table presents Genomics kernel throughput, kernel clock frequency met and resource utilization when Smith-waterman, SMEM and PairHMM algorithms are executed on Alveo and Versal boards.

+----------------------------+----------------------+----------+---------+-------+-------+--------+
| Architecture               |      Throughput      |  FMax    |   LUT   |  BRAM |  URAM |  DSP   |
+============================+======================+==========+=========+=======+=======+========+
| Smithwaterman Algorithm    |       267GCUPS       |  300MHz  |   172k  |   2   |   0   |   0    |
+----------------------------+----------------------+----------+---------+-------+-------+--------+
| SMEM Algotithm             |       2GB/s          |  300MHz  |   46k   |   70  |  70   |   0    |
+----------------------------+----------------------+----------+---------+-------+-------+--------+
| PairHMM Algorithm          |       40GCUPS        |  237MHz  |   231K  |  235  |  12   |  1610  |
+----------------------------+----------------------+----------+---------+-------+-------+--------+






Test Overview
`````````````
Here are benchmarks of the Vitis Data Genomics Library using the Vitis environment. 

Vitis Data Genomics Library
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* **Download code**

These genomics benchmarks can be downloaded from `vitis libraries <https://github.com/Xilinx/Vitis_Libraries.git>`_ ``master`` branch.

.. code-block:: bash

   git clone https://github.com/Xilinx/Vitis_Libraries.git 
   cd Vitis_Libraries
   git checkout master
   cd genomics                

* **Setup environment**

Specifying the corresponding Vitis, XRT, and path to the platform repository by running following commands.

.. code-block:: bash

   source <Vitis_Intstalled_Path>/installs/lin64/Vitis/2021.2/settings64.sh
   source <Vitis_Installed_Path>/xbb/xrt/packages/setup.sh
   export PLATFORM_REPO_PATHS=/opt/xilinx/platforms
   export LD_LIBRARY_PATH=$XILINX_VITIS/lib/lnx64.o/Default/:$LD_LIBRARY_PATH

* **Build Instructions**

Execute the following commands to compile and test run the applications.

.. code-block:: bash
      
   $ make run TARGET=hw

   hw: run on actual hardware

By default, the target device is set as Alveo U200 for Smith-waterman Algorithm, Alveo U250 SMEM algorithms and 
Versal AI VCK 190 for PairHMM. In order to target a different device, use the  ``DEVICE`` argument. For example:

.. code-block:: bash

    make run TARGET=hw DEVICE=<new_device.xpfm>

.. NOTE::
   Build instructions explained in this section are common for all the
   applications to run on actual hardware. The generated executable names may differ.
