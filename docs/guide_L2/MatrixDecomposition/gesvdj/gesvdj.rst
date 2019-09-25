
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

**********************************************************
Singular Value Decomposition for symmetric matrix (GESVDJ)
**********************************************************

Overview
========

The `singular value decomposition` (SVD) is a very useful technique for dealing with general dense matrix problems. Recent years, SVD has become a computationally viable tool for solving a wide variety of problems raised in many practical applications, such as least squares data fitting, image compression, facial recognition, principal component analysis, latent semantic analysis, and computing the 2-norm, condition number, and numerical rank of a matrix. 

For more information, please refer **"Jack Dongarra, Mark Gates, Azzam Haidar. The Singular Value Decomposition: Anatomy of Optimizing an Algorithm for Extreme Scale. 2018 SIAM Review, vol.60, No.4, pp.808-865"**


Theory
========

The SVD of an m-by-m symmetric matrix A is given by

.. math::
            A = U \Sigma U^T (A = U \Sigma U^H \, in \, the \, complex \, case)

where :math:`U` is an orthogonal (unitary) matrix and :math:`\Sigma` is an m-by-m matrix with real diagonal elements.

There are two dominant categories of SVD algorithms for dense matrix: bidiagonalization methods and Jacobi methods. The classical bidiagonalization methods is a long sequential calculation, FPGA has no advantage in calculation. In contrast, Jacobi methods apply plane rotations to the entire matrix A. Two-sided Jacobi methods iteratively apply rotations on both sides of matrix A to bring it to diagonal form, while one-sided Hestenes Jacobi methods apply rotations on one side to orthogonalize the columns of matrix A, and bring :math:`A^TA` to diagonal form. While Jacobi methods are often slow than bidiagonalization methods, they have a better potential in unrolling and pipelining. 

Jacobi Methods
--------------
Jacobi uses a sequence of plane rotations to reduce a symmetric matrix A to a diagonal matrix

.. math::
            A_{0} = A,   \> A_{k+1} = J_{k}^{T}A_{k}J_{k},   \> A_{k} \rightarrow \Sigma \> as \> k \rightarrow \infty
            


Each plane rotation, :math:`J_{k} = J_{k}(i, j, \theta)`, now called a Jacobi or Givens rotation

.. math::
            \begin{equation}
                J_{k}(i, j, \theta)=\begin{vmatrix}
                    I &\,  &  &  & \\
                    \, &\,c &  &s & \\
                    \, &\,  &I &  & \\
                    \, &-s  &  &c & \\
                    & &\,  &  &  &I
                \end{vmatrix}
            \end{equation}
    :label: Jacobi_rotation

where :math:`c=cos \theta` and :math:`s=sin \theta`. The angle :math:`\theta` is chosen to eliminate the pair :math:`a_{ij}`, :math:`a_{ji}` by applying :math:`J(i,j, \theta )` on the left and right of :math:`A`, which can be viewed as the 2x2 eigenvalue problem

.. math::
         \begin{equation}
          \hat{J}_{(k)}^{T} \hat{A}_{(k)} \hat{J}_{(k)}= \begin{vmatrix}
            \, c &s \\
              -s &c
              \end{vmatrix}^{T} \begin{vmatrix}
              a_{ii} &a_{ij} \\
              a_{ji} &a_{jj}
              \end{vmatrix} \begin{vmatrix}
              d_{ii}  &0 \\
                  0   &d_{jj}
                  \end{vmatrix}= \hat{A}_{(k+1)}
         \end{equation}

where :math:`\hat{A}` is a 2X2 submatrix of matrix A. After the Givens rotations of the whole matrix A, the off-diagonal value of A will be reduced to zero-like value after 3-15 times iteration of the process.


Benchmarking
============

The hardware resources and performance for double and float type gesvdj are listed in :numref:`tabSVDDouble` and :numref:`tabSVDFloat`. (Vivado result)

.. _tabSVDDouble:

.. table:: double Type GESVDJ performance chart
    :align: center

    +-------------+--------+------+------+-----+----------+--------+--------------+-----------+
    | Matrix Size | Unroll | URAM | BRAM | DSP | Register |  LUT   | E2E time(ms) | clock(ns) |
    +-------------+--------+------+------+-----+----------+--------+--------------+-----------+
    |    8x8      |    4   |   20 |    6 | 216 |   46245  |  39365 |      0.082   |  4.0000   |
    +-------------+--------+------+------+-----+----------+--------+--------------+-----------+
    |  512x512    |    8   |  128 |  333 | 408 |  120837  | 115121 |   2100.833   |  4.8007   |
    +-------------+--------+------+------+-----+----------+--------+--------------+-----------+

.. _tabSVDFloat:

.. table:: float Type GESVDJ performance chart
    :align: center

    +-------------+--------+------+------+-----+----------+--------+--------------+-----------+
    | Matrix Size | Unroll | URAM | BRAM | DSP | Register |  LUT   | E2E time(ms) | clock(ns) |
    +-------------+--------+------+------+-----+----------+--------+--------------+-----------+
    |    8x8      |    4   |   20 |    4 | 114 |   23647  |  18529 |      0.071   |  4.0000   |
    +-------------+--------+------+------+-----+----------+--------+--------------+-----------+
    |  512x512    |    8   |  128 |  307 | 210 |   65569  |  65003 |   1687.274   |  4.3687   |
    +-------------+--------+------+------+-----+----------+--------+--------------+-----------+

.. note::
    All the above data is measured through `Xilinx Alveo U250 Data Center Accelerator Card <https://www.xilinx.com/products/boards-and-kits/alveo/u250.html>`_ with XDMA 201830_1 platform


The accuracy of GESVDJ implementation has been verified with Lapack dgesvd (QR based SVD) and dgesvj (Jacobi SVD) functions. 

.. caution::
    The unroll factor is limited by 2 factors, the matrix size and URAM port, the maximum unroll factor should be half of matrix size, and :math:`2 \times {Unroll}^{2}` should be less than available URAM on board. Besides, unroll factor can only be a factorization of 2


.. toctree::
   :maxdepth: 1
