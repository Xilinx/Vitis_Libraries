# Vitis Solver Library

## Overview

Vitis Solver Library provides a collection of matrix decomposition operations, linear solvers and eigenvalue solvers.

Currently this includes the following operations for dense matrix
  - Matrix decomposition
    * Cholesky decomposition for symmetric positive definite matrix
    * LU decomposition without pivoting and with partial pivoting
    * QR decomposition for general matrix
    * SVD decomposition (single value decomposition) for symmetric matrix and non-symmetric matrix (Jacobi method)
  - Linear solver
    * Tridiagonal linear solver (Parallel cyclic reduction method)
    * Linear solver for triangular matrix
    * Linear solver for symmetric and non-symmetric matrix
    * Matrix inverse for symmetric and non-symmetric matrix
  - Eigenvalue solver
    * Jacobi eigenvalue solver for symmetric matrix

## Software and Hardware requirements
  - CentOS/RHEL 7.4, 7.5 or Ubuntu 16.04.4 LTS, 18.04.1 LTS
  - Vitis 2022.2
  - Alveo U200, U250, U280

## Source Files and Application Development
Vitis libraries are organized into L1, L2, and L3 folders, each relating to a different stage of application development.

**L1**:
      Makefiles and sources in L1 facilitate HLS based flow for quick checks. Tasks at this level include:

* Check the functionality of an individual kernel (C-simulation)
* Estimate resource usage, latency, etc. (Synthesis)
* Run cycle accurate simulations (Co-simulation)
* Package as IP and get final resource utilization/timing details (Export RTL)
       
	**Note**:  Once RTL (or XO file after packaging IP) is generated, the Vivado flow is invoked for XCLBIN file generation if required.

**L2**: Makefiles and sources in L2 facilitate building XCLBIN file from various sources (HDL, HLS or XO files) of kernels with host code written in OpenCL/XRT framework targeting a device. This flow supports:

* Software emulation to check the functionality
* Hardware emulation to check RTL level simulation
* Build and test on hardware

**L3**: Makefiles and sources in L3 demonstrate applications developed involving multiple kernels in pipeline. These Makefiles can be used for executing tasks, as with the L2 Makefiles.

## Benchmark Result
In `L2/benchmarks`, Kernels are built into xclbins targeting Alveo U200/U250. We achieved a good performance. For more details about the benchmarks, please kindly find them in [benchmark results](https://docs.xilinx.com/r/en-US/Vitis_Libraries/solver/benchmark.html).
  
## Documentations
For more details of the Graph library, please refer to [Vitis Solver Library Documentation](https://docs.xilinx.com/r/en-US/Vitis_Libraries/solver/index.html).

## License

 Copyright © 2019–2023 Advanced Micro Devices, Inc

Terms and Conditions <https://www.amd.com/en/corporate/copyright>
