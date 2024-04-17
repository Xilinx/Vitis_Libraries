..
   .. Copyright © 2021–2023 Advanced Micro Devices, Inc

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

Given rotations is used in the current design.


Implementation
============

DataType Supported
--------------------
* float
* x_complex<float>
* std::complex<float>

.. note::
   Subnormal values are not supported. If used, the synthesized hardware flushes these to zero, and the behavior differs versus software simulation.

Interfaces
--------------------
* Template parameters:

  * TransposedQ      : Selects whether Q is output in transposed form
  * RowsA            : Number of rows in input matrix A
  * ColsA            : Number of columns in input matrix A
  * InputType        : Input data type
  * OutputType       : Output data type
  * QRF_TRAITS       : qrfTraits type with specified values

* Arguments:

  * matrixAStrm      : Stream of Input matrix
  * matrixQStrm      : Stream of Orthogonal output matrix
  * matrixRStrm      : Stream of Upper triangular output matrix

.. note::
   The function will fail to compile or synthesize if **RowsA < ColsA**.


Implementation Controls
------------------------

Specifications
~~~~~~~~~~~~~~~~~~~~~~~~~
There is a configuration class derived from the base configuration class **xf::solver::qrfTraits** by redefining the appropriate class member.

.. code::

   struct my_qrf_traits : xf::solver::qrfTraits<A_ROWS, A_COLS, MATRIX_IN_T, MATRIX_OUT_T> {
       static const int ARCH = SEL_ARCH;
   };

The base configuration class is:

.. code::

   template <int RowsA, int ColsA, typename InputType, typename OutputType>
   struct qrfTraits {
       static const int ARCH = 1;         
       static const int CALC_ROT_II = 1; 
       static const int UPDATE_II = 4;    
       static const int UNROLL_FACTOR =1; 
   };

.. note::
   * ARCH:          Select implementation. 0=Basic. 1=Lower latency/thoughput architecture.
   * CALC_ROT_II:   Specify the rotation calculation loop target II of the QRF_ALT architecture(1).
   * UPDATE_II:     Specify the pipelining target for the Q & R update loops.
   * UNROLL_FACTOR: Specify the unrolling factor for Q & R update loops of the QRF_ALT architecture(1).

The configuration class is supplied to the **xf::solver::qrf** function as a template paramter as follows.

.. code::

    template <bool TransposedQ,
              int RowsA,
              int ColsA,
              typename InputType,
              typename OutputType,
              typename QRF_TRAITS = DEFAULT_QRF_TRAITS>
    void qrf(hls::stream<InputType>& matrixAStrm,
             hls::stream<OutputType>& matrixQStrm,
             hls::stream<OutputType>& matrixRStrm) 


Key Factors
~~~~~~~~~~~~~~~~~~~~~~~~~
The following table summarizes how the key factors from the configuration class influence resource utilization, function throughput (initiation interval), and function latency. The values of Low, Medium, and High are relative to the other key factors.  

.. table:: QRF Key Factor Summary   
    :align: center

    +------------------+-------+-----------+------------+----------+
    |    Key Factor    | Value | Resources | Throughput | Latency  |
    +==================+=======+===========+============+==========+
    | Q and R update   |   2   |   High    |    High    |  Low     |   
    | loop pipelining  +-------+-----------+------------+----------+   
    | (UPDATE_II)      |   >2  |   Low     |    Low     |  High    |
    +------------------+-------+-----------+------------+----------+
    | Q and R update   |   1   |   Low     |    Low     |  High    |
    | unrolling        +-------+-----------+------------+----------+   
    | (UNROLL_FACTOR)  |   >1  |   High    |    High    |  Low     |
    +------------------+-------+-----------+------------+----------+
    | Rotation loop    |   1   |   High    |    High    |  Low     |   
    | pipelining       +-------+-----------+------------+----------+   
    | (CALC_ROT_II)    |   >1  |   Low     |    Low     |  High    |
    +------------------+-------+-----------+------------+----------+

.. Note::   
  * Q and R update loop pipelining: Sets the achievable initiation interval (II);   
  * Q and R update loop unrolling:  Duplicate hardware when implement loop processing, execute corresponding number of loop iterations in parallel;   
  * Rotation loop pipelining:       Enables AMD Vivado |trade| HLS to share resources and reduce the DSP utilization

  

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:


