..
   .. Copyright © 2021–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: QRD
   :description: QR Decomposition
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

*******************************************************
QRD (QR Decomposition)
*******************************************************

Overview
============
QR decomposition, is a decomposition of a matrix :math:`A` into a product of an orthogonal matrix :math:`Q` and an upper triangular matrix :math:`R`. 

This API shows a high performance design of QRD in Versal device. For complex float 1024*256 matrix, this design can achieve 790+ GFLOPS on VCK190.

For DSP performance, nearly 100% sustained to peak performance is achieved.

This design structure is highly scalable. In the smaller dimension of 256*64, resources and performance are linearly related to the case of 1024*256.

QRD is often used to solve the linear least squares problem and is the basis for a particular eigenvalue algorithm, the QR algorithm.

.. math::
            A = Q R

There are several methods for actually computing the QR decomposition, such as by means of the Gram-Schmidt process, Householder transformations, or Givens rotations. Each has a number of advantages and disadvantages.

In our design, Gram-Schmidt is used.


Implementation
============

DataType Supported
--------------------
* float
* x_complex<float>

.. note::
   Subnormal values are not supported. If used, the synthesized hardware flushes these to zero, and the behavior differs versus software simulation.

Interfaces
--------------------
* Template parameters:

  * RowsA            : Number of rows in input matrix A
  * ColsA            : Number of columns in input matrix A
  * PowUnroll        : Power2 of RowsA Size
  * PowFoldRow       : Power2 of fold Rows Size
  * NCU              : Number of Compute Unit
  * PowNCU           : Power2 of compute unit(CU) number
  * T                : Input/output data type

* Arguments:

  * dataA            : Input port, Matrix A as input and output matrix Q
  * R_strm           : Output port, Matrix R, non-zero numbers in the upper triangular matrix

.. Note::
   The function fails to compile or synthesize if **RowsA < ColsA**.
   For multi-cu design, expand Row into two dimensions[NCU][RowsA / NCU], NCU is related to PowNCU.


Implementation Controls
------------------------

Specifications
~~~~~~~~~~~~~~~~~~~~~~~~~
There is a configuration defined the kernel implement cpp file.

.. code::

   #if (QRF_A_ROWS <= 1024 && QRF_A_ROWS > 512)
   const int PowUnroll_t = 10;
   const int PowNCU_t = 5;
   #elif (QRF_A_ROWS <= 512 && QRF_A_ROWS > 256)
   const int PowUnroll_t = 9;
   const int PowNCU_t = 5;
   #elif (QRF_A_ROWS <= 256 && QRF_A_ROWS > 128)
   const int PowUnroll_t = 8;
   const int PowNCU_t = 5;
   #elif (QRF_A_ROWS <= 128 && QRF_A_ROWS > 64)
   const int PowUnroll_t = 7;
   const int PowNCU_t = 2;
   #elif (QRF_A_ROWS <= 64 && QRF_A_ROWS > 32)
   const int PowUnroll_t = 6;
   const int PowNCU_t = 2;
   #elif (QRF_A_ROWS <= 32 && QRF_A_ROWS > 16)
   const int PowUnroll_t = 5;
   const int PowNCU_t = 2;
   #elif (QRF_A_ROWS <= 16 && QRF_A_ROWS > 8)
   const int PowUnroll_t = 4;
   const int PowNCU_t = 2;
   #elif (QRF_A_ROWS <= 8 && QRF_A_ROWS > 4)
   const int PowUnroll_t = 3;
   const int PowNCU_t = 1;
   #endif

   const int POWFoldRow_t = 2;
   const int NCU_t = 1 << PowNCU_t;
   const int UnrollSize_t = 1 << (PowUnroll_t - POWFoldRow_t);

So this kernel could automatically deduce the right configuration (PowUnroll_t and PowNCU_t)
when set to appropriate input matrix rows and columns. 

You can also set a parameter list suitable for your case according to the logical relationship of these parameters.

The base configuration class is:

.. code::

   template <1024, 256, 10, 2, 32, 5, float>


Key Factors
~~~~~~~~~~~~~~~~~~~~~~~~~
The following table summarizes the QRD for complex float performance and resources Summary.  

.. table:: QRD performance and resources Summary   
    :align: center

    +------------------+-----------+-------------+-----------+-----------+----------------------+------------+-----------+-------+-----------+
    |    Platform      |   Matrix  |    LUT      |   DSP     |   Freq.   | Latency for 1 matrix | matrix/sec |  GFLOPS   |   W   | GFLOPS/W  |
    +==================+===========+=============+===========+===========+======================+============+===========+=======+===========+
    |   Versal core    | 1024*256  | 247.8k(29%) | 1584(79%) |   422MHz  |       285,976        |   1.48k    |   793     | 17.6* |   45.1    |  
    +------------------+-----------+-------------+-----------+-----------+----------------------+------------+-----------+-------+-----------+  
    | Versal permium   | 1024*256  | 185.4k(22%) | 3258(79%) |   388MHz  |       147,714        |   2.62k    |   1412    | 32.5* |   43.5    |
    +------------------+-----------+-------------+-----------+-----------+----------------------+------------+-----------+-------+-----------+

.. Note::   
  * Estimate the dynamic power by AMD Vivado |trade| 23.2. Confidence level is Medium in Vivado power report.

  

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
