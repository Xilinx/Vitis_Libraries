.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

***************************************************************
GEMV-based Conjugate Gradient Solver with Jacobi Preconditioner
***************************************************************

Introduction
############

CG solver is widely adopted to solve linear system Ax=b, where the matrix A is symmetric and positive definite. 
Here is the benchmark for GEMV-based CG solver with the Jacobi preconditioner on AMD FPGA AMD Alveo |trade| U50. 

Executable Usage
#################

Environment Setup (Step 1)
******************************
Follow the page :doc:`Benchmark Overview <../../../benchmark>` to correctly setup the environment first.  

Build Kernel (Step 2)
******************************

With the following commands, kernel bitstream *cgSolver.xclbin* is built under the directory *./build_dir.hw.xilinx_u50_gen3x16_xdma_201920_3*

.. code-block:: bash

    $ make build TARGET=hw PLATFORM=xilinx_u50_gen3x16_xdma_201920_3

Prepare Data (Step 3)
******************************
To benchmark the kernel, there are two ways to prepare the data. 

Randomly-Generated Data (Optional)
=======================================
Safely skip this step as it is integrated with the one in the next step if you choose to use random data for the benchmark. 
Here states the principle of how it works.  With the following commands with given vector size, for example, 1024,  three data files are generated under directory *./build_dir.hw.xilinx_u50_gen3x16_xdma_201920_3/data/*.  

1.	It generates a random **SPD** matrix of size *NxN* with data type FP64 and then stores the data in a row-major to file *A.mat*.
2.	It generates a random FP64 vector of size *N* and then compute vector *b = Ax*.
3.	The two vectors are stored into files *x.mat* and *b.mat*, respectively. 

Matrix *A* and vector *b* are used as inputs for the solver, and vector *x* is used as the golden reference. 

.. code-block:: bash

    $ make data_gen TARGET=hw PLATFORM=xilinx_u50_gen3x16_xdma_201920_3 N=1024

where *N* is the vector size and must be multiple of 16.

Users' data
==================

You could prepare your data for benchmark. 
1.	Prepare a **SPD** matrix with double precision floating point data type.
2.	Prepare golden reference vector and result vector which is the product of the matrix and the golden reference.
3.	Make sure the matrix size is *NxN* and vector size is *N*
4.	Make sure*N* is multiple of 16.
5.	Store the matrix, golden reference vector and result vector to binary files named  *A.mat*, *x.mat* and *b.mat* respectively, and place them into a directory.

Run on FPGA with Example Data (Step 4)
******************************************

Check Device
===============

If you followed the guide and correctly setup the environment, you are able to run the following command line. You could check whether the target device is prepared and find out the device ID. 

.. code-block:: bash

    $ xbutil scan

Benchmark Random Dataset
=========================

If you decide to use randomly generated data for benchmark in step 3, you could skip that step and run the following command with given vector size *N*, for example, 1024 and maximum number of iterations for the solver, for example, 100. 

.. code-block:: bash

    $ make run TARGET=hw PLATFORM=xilinx_u50_gen3x16_xdma_201920_3 N=1024 maxIter=100 deviceID=0

The following table lists the configurable parameters with the *make* command for the benchmark. 

.. table:: Parameters with make command 
    :align: center

    +----------------+---------------+----------------------------------------------+
    | Parameter Name | Default Value | Notes                                        |
    +================+===============+==============================================+
    | N              | 1024          | Vector size, must be multiple of 16          |
    +----------------+---------------+----------------------------------------------+
    | maxIter        | 100           | Maximum No. iterations for the solver <= 2000|
    +----------------+---------------+----------------------------------------------+
    | tol            | 1e-12         | Fault Tolerance                              |
    +----------------+---------------+----------------------------------------------+
    | deviceID       | 0             | Alveo U50 Card ID                            |
    +----------------+---------------+----------------------------------------------+
    | condition_num  | 128           | Conditioner number for matrix generated      |
    +----------------+---------------+----------------------------------------------+



Usage
==============
For users' own data, follow the usage specified bellow. 

.. code-block:: bash

    Usage: host.exe <XCLBIN File> <Max Iteration> <Vector Size> <DATA PATH> [device id]
                <XCLBIN File>       path to the xclbin file
                <Max Iteration>     maximum number of iterations
                <Tolerence>         Fault tolerence
                <Vector Size>       size of vector, matrix size N x N
                <DATA PATH>         path to the matrix and vector binary files
                <device id>         Device id given


Resource Utilization
########################

The following table lists the resource utilization for GEMV-based CG kernel with 16 HBM channels storing the matrix. 

.. table:: Resource Utilization on U50
    :align: center

    +----------------------------+------------------+------------------+-------------------+----------------+---------------+----------------+
    | Name                       | LUT              | LUTAsMem         | REG               | BRAM           | URAM          | DSP            |
    +============================+==================+==================+===================+================+===============+================+
    | User Budget                | 699619 [100.00%] | 369603 [100.00%] | 1447189 [100.00%] | 1112 [100.00%] | 640 [100.00%] | 5936 [100.00%] |
    +----------------------------+------------------+------------------+-------------------+----------------+---------------+----------------+
    |    Used Resources          | 186448 [ 26.65%] |  17334 [  4.69%] |  325149 [ 22.47%] | 128 [ 11.51%]  |   0 [  0.00%] | 1262 [ 21.26%] |
    +----------------------------+------------------+------------------+-------------------+----------------+---------------+----------------+


Benchmark Results on Alveo U50 FPGA
####################################

CPU Hardware information

*   Model name: Intel(R) Xeon(R) CPU E5-2667 v4 @ 3.20GHz
*   Total threads: 32, Threads/Core: 2, Cores/Socket: 8, Total sockets: 2, Total Cores:16

FPGA Hardware Information

* Device name:  Xilinx Alveo U50
* Fmax: 333MHz
* Idle power 24W

.. table:: Benchmark Results on U50
    :align: center

    +-------------+-------------------------+---------------------------+----------------------------------+--------------------------+--------------------+
    | Vector Size | Time per Iteration [ms] | U50 Performance [GFLOPS]  | U50 Energy Efficiency [GFLOPS/W] | CPU Performance [GFLOPS] | Acceleration Ratio |
    +=============+=========================+===========================+==================================+==========================+====================+
    |    1024     |    0.073                | 26.938                    |    0.723                         |    12.996                | 2.073              |
    +-------------+-------------------------+---------------------------+----------------------------------+--------------------------+--------------------+
    |    2048     |    0.2557               | 30.658                    |    0.766                         |    27.469                | 1.116              |
    +-------------+-------------------------+---------------------------+----------------------------------+--------------------------+--------------------+
    |    4096     |    0.9202               | 34.018                    |    0.812                         |    7.776                 | 4.375              |
    +-------------+-------------------------+---------------------------+----------------------------------+--------------------------+--------------------+
    |    8192     |    3.405                | 36.742                    |    0.839                         |    8.226                 | 4.467              |
    +-------------+-------------------------+---------------------------+----------------------------------+--------------------------+--------------------+



Power Consumption on FPGA
*****************************
Power data could be obtained by 

.. code-block:: bash

    $ xbutil top -d <PLATFORM ID>

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim: