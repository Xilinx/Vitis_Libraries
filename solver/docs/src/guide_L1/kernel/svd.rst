..
   .. Copyright © 2021–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: SVD 
   :description: SVD Factorization
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

*******************************************************
SVD (Singular Value Decomposition)
*******************************************************

Overview
============
SVD is a factorization of a real or complex matrix. It generalizes the eigen decomposition of a square normal matrix with an orthonormal eigenbasis to any :math:`m\times n` matrix. It is related to the polar decomposition. 

The singular value decomposition of a :math: `m\times n` complex matrix :math:`M` is a factorization of the form :math:`U`:math:`\Sigma`:math:`V^*`, where :math:`U` is a :math:`m\times m` complex unitary matrix, :math:`\Sigma` is a :math:`m\times n` rectangular diagonal matrix with non-negative real numbers on the diagonal, and :math:`V` is a :math:`n\times n` complex unitary matrix. If :math:`M` is real, :math:`U` and :math:`V` can also be guaranteed to be real orthogonal matrix.

.. math::
            M = U{\Sigma}V^* 

The diagonal entries :math:`\Sigma_{ii}` of :math:`\Sigma` are known as the singular values of :math:`M`. The number of non-zero singular values is equal to the rank of :math:`M`. the Columns of :math:`U` and :math:`V` are called the left-singular vectors and right-singular vectors of :math:`M`, respectively.


Implementation
===============
The singular value decomposition of input matrix :math:`A` is computed and matrix :math:`U`, :math:`S`, :math:`V` is generated.

.. math::
           A = USV^*

In this design, only square matrix is supported.
The `iterative two-sided Jacobi` method is used in this API.

DataType Supported
--------------------
* float
* x_complex<float>
* std::complex<float>

Interfaces
-------------------- 
* Template parameters:

    * RowsA                 Row dimension
    * ColsA                 Column dimension
    * InputType             Input data type
    * OutputType            Output data type
    * SVDTraits             SVDTraits class

* Arguments:

    * matrixAStrm           Stream of input matrix
    * matrixSStrm           Stream of singular values input matrix
    * matrixUStrm           Stream of left singular vectors input matrix
    * matrixVStrm           Stream of right singular vectors input matrix
  
.. Note::
   * The function throws an assertion and fails to compile or synthesize, if **RowsA != ColsA**.
   * For floating point types, subnormal input values are not supported. If used, the synthesized hardware flushes these to zero, and behavior differs versus software simulation.


Implementation Controls
------------------------

Specifications
~~~~~~~~~~~~~~~~~~~~~~~~~
There is a configuration class derived from the base configuration class **xf::solver::svdTraits** by redefining the appropriate class member.

.. code::

   struct my_svd_config : xf::solver::svdTraits<ROWS, COLS, MATRIX_IN_T, MATRIX_OUT_T> {
       static const int ARCH = SEL_ARCH;
   };


The base configuration class is:

.. code::

   template <int RowsA, int ColsA, typename InputType, typename OutputType>
   struct svdTraits {
       typedef OutputType SIntType;
       typedef OutputType UIntType;
       typedef OutputType VIntType;
       typedef OutputType CSIntType;
       static const int NUM_SWEEPS = 10; 
       static const int MIN_DIM = (RowsA < ColsA ? RowsA : ColsA);
       static const int ARCH = 1;        
       static const int OFF_DIAG_II = 8; 
       static const int DIAG_II = 8; 
   };


.. note::
   * NUM_SWEEPS:  The SVD function uses the iterative two-sided Jacobi method. Literature typically suggests 6 to 10 iterations to successfully converge.
   * ARCH:        Select implementation. 0 = Basic loop engine. 1 = Pairs based engine.
   * OFF_DIAG_II: Specify the pipelining target for the off diagonal loop. 
   * DIAG_II:     Specify the pipelining target for the diagonal loop. 

The configuration class is supplied to the **xf::solver::svd** function as a template parameter as follows.

.. code::

   template <int RowsA,
             int ColsA,
             typename InputType,
             typename OutputType,
             typename SVDTraits = svdTraits<RowsA, ColsA, InputType, OutputType> >
   svd(hls::stream<InputType >& matrixAStrm,
       hls::stream<OutputType>& matrixSStrm,
       hls::stream<OutputType>& matrixUStrm,
       hls::stream<OutputType>& matrixVStrm)


Key Factors
~~~~~~~~~~~~~~~~~~~~~~~~~
The following table summarizes that how the key factors from the configuration class influence resource utilization, function throughput (initiation interval), and function latency. The values of Low, Medium, and High are relative to the other key factors.

.. table:: SVD Key Factor Summary  
    :align: center

    +------------------+-------+-----------+------------+----------+
    |    Key Factor    | Value | Resources | Throughput | Latency  |
    +==================+=======+===========+============+==========+
    | Iterations       |  <10  |   N/A     |    High    |  Low     |    
    | (NUM_SWEEP)      |       |           |            |          |    
    +------------------+-------+-----------+------------+----------+
    | Off-diagonal loop|   4   |   High    |    High    |  Low     |    
    | pipelining       +-------+-----------+------------+----------+
    | (OFF_DIAG_II)    |   >4  |   Low     |    Low     |  High    |    
    +------------------+-------+-----------+------------+----------+
    | Diagonal loop    |   1   |   High    |    High    |  Low     |    
    | pipeling         +-------+-----------+------------+----------+    
    | (DIAG_II)        |   >1  |   Low     |    Low     |  High    |    
    +------------------+-------+-----------+------------+----------+

.. Note::
  * Iterations: The SVD function uses the iterative two-sided Jacobi method. The default number of iterations is 10.
  * Off-diagonal loop pipelining:  the minimum achievable initiation interval (II) is 4, which satisfies the S, U, and V array requirement of four writes every iteration of the off-diagonal loop.
  * Diagonal loop pipelining: value >1, enables AMD Vivado |trade| HLS to resource share 

  
.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:


