..
   .. Copyright © 2021–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.
.. meta::
   :keywords: Cholesky
   :description: Cholesky Decomposition
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

*******************************************************
Cholesky 
*******************************************************

Overview
============
Cholesky decomposition is a decomposition of a Hermitian, positive-definite matrix into the product of a lower triangular matrix and its conjugate transpose, in the form of :math:`A = LL^*`. :math:`A` is a Hermitian positive-definite matrix, :math:`L` is a lower triangular matrix with real and positive diagonal entries, and :math:`L^*` denotes the conjugate transpose of :math:`L`. 
Cholesky decomposition is useful for efficient numerical solutions. 

.. math::

            A = L*L^* 

Implementation
============

DataType Supported
--------------------
* float
* x_complex<float>
* std::complex<float>
* ap_fixed
* x_complex<ap_fixed>
* std::complex<ap_fixed>

.. Note::
   * It's recommended to use floating point data types.
   * Support of "ap_fixed", "x_complex<ap_fixed>" and "std::complex<ap_fixed>" will be deprecated in the future.
   * The function assumes that the input matrix is a symmetric positive definite (Hermitian positive definite for complex-valued inputs).
   * Subnormal values are not supported. If used, the synthesized hardware flushes these to zero, and the behavior differs versus software simulation.

Interfaces
--------------------
* Template parameters:

  *  RowsColsA              Defines the matrix dimensions
  *  InputType              Input data type
  *  OutputType             Output data type
  *  TRAITS                 Cholesky traits class
   
* Arguments:

  * matrixAStrm             Stream of Square Hermitian/symmetric positive definite input matrix
  * matrixLStrm             Stream of Lower or upper triangular output matrix 

* Return Values: 

  * 0 = Success. 
  * 1 = Failure. The function attempted to find the square root of a negative number, that is, the input matrix A was not Hermitian/symmetric positive definite.


Implementation Controls
------------------------

Specifications
~~~~~~~~~~~~~~~~~~~~~~~~~
There is a configuration class derived from the base configuration class **xf::solver::choleskyTraits** by redefining the appropriate class member.

.. code::

   struct my_cholesky_traits : xf::solver::choleskyTraits<LOWER_TRIANGULAR, DIM, MATRIX_IN_T, MATRIX_OUT_T> {
       static const int ARCH = SEL_ARCH;
   };

The default base configuration class is as following. If the input datatype is complex or ap_fixed, refer to **L1/include/hw/cholesky.hpp** for more details.

.. code::

   template <bool LowerTriangularL, int RowsColsA, typename InputType, typename OutputType>
   struct choleskyTraits {
       typedef InputType PROD_T;
       typedef InputType ACCUM_T;
       typedef InputType ADD_T;
       typedef InputType DIAG_T;
       typedef InputType RECIP_DIAG_T;
       typedef InputType OFF_DIAG_T;
       typedef OutputType L_OUTPUT_T;
       static const int ARCH = 1;
       static const int INNER_II = 1;
       static const int UNROLL_FACTOR = 1; 
       static const int UNROLL_DIM = (LowerTriangularL == true ? 1 : 2);
       static const int ARCH2_ZERO_LOOP = true; 
   };

.. note::
   * ARCH:            Select implementation: 0=Basic, 1=Lower latency architecture, 2=Further improved latency architecture
   * INNER_II:        Specify the pipelining target for the inner loop
   * UNROLL_FACTOR:   The inner loop unrolling factor for the choleskyAlt2 architecture(2) to increase throughput
   * UNROLL_DIM:      Dimension to unroll matrix
   * ARCH2_ZERO_LOOP: Additional implementation "switch" for the choleskyAlt2 architecture (2).

The configuration class is supplied to the **xf::solver::cholesky** function as a template parameter as follows.

.. code::

   template <bool LowerTriangularL,
             int RowsColsA,
             class InputType,
             class OutputType,
             typename TRAITS = choleskyTraits<LowerTriangularL, RowsColsA, InputType, OutputType> >
   int cholesky(hls::stream<InputType>& matrixAStrm, hls::stream<OutputType>& matrixLStrm)

Key Factors
~~~~~~~~~~~~~~~~~~~~~~~~~
The following table summarizes how the key factors from the configuration class influence resource utilization, function throughput (initiation interval), and function latency. The values of Low, Medium, and High are relative to the other key factors.  

.. table:: Cholesky Key Factor Summary   
    :align: center

    +------------------+-------+-----------+------------+----------+
    |    Key Factor    | Value | Resources | Throughput | Latency  |
    +==================+=======+===========+============+==========+
    | Architecture     |   0   |   Low     |    Low     |  High    |
    | (ARCH)           +-------+-----------+------------+----------+   
    |                  |   1   |   Medium  |    Medium  |  Medium  |
    |                  +-------+-----------+------------+----------+   
    |                  |   2   |   High    |    High    |  Low     |
    +------------------+-------+-----------+------------+----------+
    | Inner loop       |   1   |   High    |    High    |  Low     |   
    | pipeling         +-------+-----------+------------+----------+   
    | (INNER_II)       |   >1  |   Low     |    Low     |  High    |
    +------------------+-------+-----------+------------+----------+
    | Inner loop       |   1   |   Low     |    Low     |  High    |
    | unrolling        +-------+-----------+------------+----------+   
    | (UNROLL_FACTOR)  |   >1  |   High    |    High    |  Low     |
    +------------------+-------+-----------+------------+----------+

.. Note::   
   * Architecture

     * 0: Uses the lowest DSP utilization and lowest throughput.
     * 1: Uses higher DSP utilization but minimized memory utilization with increased throughput. This value does not support inner loop unrolling to further increase throughput.
     * 2: Uses highest DSP and memory utilization. This value supports inner loop unrolling to improve overall throughput with a limited increase in DSP resources. This is the most flexible architecture for design exploration.
   * Inner loop pipeling

     * >1: For ARCH 2, enables resource share and reduces the DSP utilization. When using complex floating-point data types, setting the value to two or four significantly reduces DSP utilization.
   * Inner loop unrolling

     * For ARCH 2, duplicates the hardware required to implement the loop processing by a specified factor, executes the corresponding number of loop iterations in parallel, and increases throughput but also increases DSP and memory utilization.

