# XF Solver Library

## Overview

XF Solver Library provides a collection of matrix decomposition operations, linear solvers and eigen values solvers.

Currently this includes following operations for dense matrix
  - Matrix decomposition
    * Cholesky decomposition for symmetric positive defined matrix
    * LU decomposition without pivoting and with partial pivoting
    * QR decomposition
    * SVD decomposition (single value decomposition) for symmetric matrix and non-symmetric matrix (Jacobi method)
    * Matrix inverse
  - Linear solver
    * Tri-diagonal linear solver (Parallel cyclic reduction method)
    * Linear solver for symmetric and non-symmetric matrix
  - Eigen value solver
    * Jacobi eigen value solver for symmetric matrix

## Software and Hardware requirements
  - CentOS/RHEL 7.4, 7.5 or Ubuntu 16.04 LTS, 18.04 LTS
  - Vitis 2019.2
  - Alveo U200, U250

## License

Licensed using the [Apache 2.0 license](https://www.apache.org/licenses/LICENSE-2.0).

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
    Copyright 2019 Xilinx, Inc.
