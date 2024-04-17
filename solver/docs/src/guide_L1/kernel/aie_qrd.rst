..
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: QRF
   :description: QR Factorization
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

*******************************************************
QRF (QR Factorization)
*******************************************************

Overview
============
QRF, also known as QR decomposition, is a decomposition of a matrix :math:`A` into a product of an orthogonal matrix :math:`Q` and an upper triangular matrix :math:`R`. 

QRF is often used to solve the linear least squares problem and is the basis for a particular eigenvalue algorithm, the QR algorithm.

.. math::
            A = Q R

There are several methods to compute the QR decomposition, such as by means of the Gram-Schmidt process, Householder transformations, or Given rotations. Each has a number of advantages and disadvantages.

Given rotations are used in the design.


Implementation
============

DataType Supported
--------------------
* float


Using Givens Rotation
-----------------------

QR decomposition could be computed with a series of Givens Rotation, which is multiplied by a series of rotation matrix.
Each rotation zeroes an element in the subdiagonal of the matrix, which finally produces the matrix R.

For matrix A of N x N, zero element in column A[:][0], then A[:][1], A[:][2] ... A[:][N-2].
For each column A[:][j], zero element from A[N-1][j], then A[N-2][j], A[N-3][j] ... A[j+1][j].
Let matrix G(i,j, c, s) of N x N be Givens rotation matrix to zero element A[i][j], then A = A * G(i, j, c, s).
Parameter c and s are scalar derived from A[i][j] and A[j][j].
G(i, j, c, s) = I + H(i, j, c, s). H is the matrix whose elements are all zeros except for H[i][i], H[i][j], H[j][i] and H[j][j].

In such constructions, calculate A = A * G(i, j, c, s), will only update A[i][:] and A[j][:]:
A[i][:] = A[i][:] * (-s) + A[j][:] * c,  A[j][:] = A[i][:] * c + A[j][:] * s.
Due to the construction of Givens Rotation, it does not need to compute whole matrix multiply of N x N, but four vectors multiply of N x 1 and 2 vector addition of N x 1.

Thus, theour implementation of QRD has two components:
(1) PL datamover to load and update matrix A in place.
(2) AIE graph to take input from PL datamover and feed updated result back to PL datamover.
Store the input and intermediate result in DDR, which helps solver bigger matrix than just using URAM on PL or data memory on AIE.
In such an arrangement, system bottleneck is the bandwidth between DDR and AIE array.

.. image:: /images/aie_qrd.png
   :alt: aie qrd
   :width: 100%
   :align: center
