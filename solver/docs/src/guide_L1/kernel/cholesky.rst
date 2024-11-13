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
   * The function assumes that the input matrix is a symmetric positive definite (Hermitian positive definite for complex-valued inputs).
   * For floating point types, subnormal input values are not supported. If used, the synthesized hardware flushes these to zero, and the behavior differs versus software simulation.

Interfaces
--------------------
* Template parameters:

   -  RowsColsA              Defines the matrix dimensions
   -  InputType              Input data type
   -  OutputType             Output data type
   -  TRAITS                 Cholesky traits class
   
* Arguments:

    - matrixAStrm             Stream of Square Hermitian/symmetric positive definite input matrix
    - matrixLStrm             Stream of Lower or upper triangular output matrix 

* Return Values:  

    - 0: Success. 
    - 1: Failure. The function attempted to find the square root of a negative number, that is, the input matrix A was not Hermitian/symmetric positive definite.


Implementation Controls
------------------------

Specifications
~~~~~~~~~~~~~~~~~~~~~~~~~
There is a configuration class derived from the base configuration class **xf::solver::choleskyTraits** by redefining the appropriate class member.

.. code::

   struct my_cholesky_traits : xf::solver::choleskyTraits<LOWER_TRIANGULAR, DIM, MATRIX_IN_T, MATRIX_OUT_T> {
       static const int ARCH = SEL_ARCH;
   };

The configuration class is supplied to the **xf::solver::cholesky** function as a template parameter as follows:

.. code::

    xf::solver::cholesky<LOWER_TRIANGULAR,ROWS_COLS_A,MAT_IN_T,MAT_OUT_T, my_cholesky_traits>(A,L);
    
The definition of **xf::solver::choleskyTraits** is in the file **L1/include/hw/cholesky.hpp** .

The default **xf::solver::choleskyTraits** struct defining the internal variable types for the cholesky function, showed as follows:

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


If the input datatype is **hls::x_complex**, the **choleskyTrais** is defined as bellow:

.. code::

    template <bool LowerTriangularL, int RowsColsA, typename InputBaseType, typename OutputBaseType>
    struct choleskyTraits<LowerTriangularL, RowsColsA, hls::x_complex<InputBaseType>, hls::x_complex<OutputBaseType> > {
        typedef hls::x_complex<InputBaseType> PROD_T;
        typedef hls::x_complex<InputBaseType> ACCUM_T;
        typedef hls::x_complex<InputBaseType> ADD_T;
        typedef hls::x_complex<InputBaseType> DIAG_T;
        typedef InputBaseType RECIP_DIAG_T;
        typedef hls::x_complex<InputBaseType> OFF_DIAG_T;
        typedef hls::x_complex<OutputBaseType> L_OUTPUT_T;
        static const int ARCH = 1;
        static const int INNER_II = 1;
        static const int UNROLL_FACTOR = 1;
        static const int UNROLL_DIM = (LowerTriangularL == true ? 1 : 2);
        static const int ARCH2_ZERO_LOOP = true;
    };
    
If the input datatype is **std::complex**, the **choleskyTrais** is defined as bellow:

.. code::

    template <bool LowerTriangularL, int RowsColsA, typename InputBaseType, typename OutputBaseType>
    struct choleskyTraits<LowerTriangularL, RowsColsA, std::complex<InputBaseType>, std::complex<OutputBaseType> > {
        typedef std::complex<InputBaseType> PROD_T;
        typedef std::complex<InputBaseType> ACCUM_T;
        typedef std::complex<InputBaseType> ADD_T;
        typedef std::complex<InputBaseType> DIAG_T;
        typedef InputBaseType RECIP_DIAG_T;
        typedef std::complex<InputBaseType> OFF_DIAG_T;
        typedef std::complex<OutputBaseType> L_OUTPUT_T;
        static const int ARCH = 1;
        static const int INNER_II = 1;
        static const int UNROLL_FACTOR = 1;
        static const int UNROLL_DIM = (LowerTriangularL == true ? 1 : 2);
        static const int ARCH2_ZERO_LOOP = true;
    };
   
If the input datatype is **ap_fixed**, the **choleskyTrais** is defined as bellow:
   
.. code::

    template <bool LowerTriangularL,
              int RowsColsA,
              int W1,
              int I1,
              ap_q_mode Q1,
              ap_o_mode O1,
              int N1,
              int W2,
              int I2,
              ap_q_mode Q2,
              ap_o_mode O2,
              int N2>
    struct choleskyTraits<LowerTriangularL, RowsColsA, ap_fixed<W1, I1, Q1, O1, N1>, ap_fixed<W2, I2, Q2, O2, N2> > {
        typedef ap_fixed<W1 + W1, I1 + I1, AP_RND_CONV, AP_SAT, 0> PROD_T;
        typedef ap_fixed<(W1 + W1) + BitWidth<RowsColsA>::Value,
                         (I1 + I1) + BitWidth<RowsColsA>::Value,
                         AP_RND_CONV,
                         AP_SAT,
                         0>
            ACCUM_T;
        typedef ap_fixed<W1 + 1, I1 + 1, AP_RND_CONV, AP_SAT, 0> ADD_T;
        typedef ap_fixed<(W1 + 1) * 2, I1 + 1, AP_RND_CONV, AP_SAT, 0> DIAG_T;     // Takes result of sqrt
        typedef ap_fixed<(W1 + 1) * 2, I1 + 1, AP_RND_CONV, AP_SAT, 0> OFF_DIAG_T; // Takes result of /
        typedef ap_fixed<2 + (W2 - I2) + W2, 2 + (W2 - I2), AP_RND_CONV, AP_SAT, 0> RECIP_DIAG_T;
        typedef ap_fixed<W2, I2, AP_RND_CONV, AP_SAT, 0>
            L_OUTPUT_T; // Takes new L value.  Same as L output but saturation set
        static const int ARCH = 1;
        static const int INNER_II = 1;
        static const int UNROLL_FACTOR = 1;
        static const int UNROLL_DIM = (LowerTriangularL == true ? 1 : 2);
        static const int ARCH2_ZERO_LOOP = true;
    };
   
   

If the input datatype is **hls::x_complex<ap_fixed>**, the **choleskyTrais** is defined as bellow:

.. code::

    template <bool LowerTriangularL,
              int RowsColsA,
              int W1,
              int I1,
              ap_q_mode Q1,
              ap_o_mode O1,
              int N1,
              int W2,
              int I2,
              ap_q_mode Q2,
              ap_o_mode O2,
              int N2>
    struct choleskyTraits<LowerTriangularL,
                          RowsColsA,
                          hls::x_complex<ap_fixed<W1, I1, Q1, O1, N1> >,
                          hls::x_complex<ap_fixed<W2, I2, Q2, O2, N2> > > {
        typedef hls::x_complex<ap_fixed<W1 + W1, I1 + I1, AP_RND_CONV, AP_SAT, 0> > PROD_T;
        typedef hls::x_complex<ap_fixed<(W1 + W1) + BitWidth<RowsColsA>::Value,
                                        (I1 + I1) + BitWidth<RowsColsA>::Value,
                                        AP_RND_CONV,
                                        AP_SAT,
                                        0> >
            ACCUM_T;
        typedef hls::x_complex<ap_fixed<W1 + 1, I1 + 1, AP_RND_CONV, AP_SAT, 0> > ADD_T;
        typedef hls::x_complex<ap_fixed<(W1 + 1) * 2, I1 + 1, AP_RND_CONV, AP_SAT, 0> > DIAG_T;     // Takes result of sqrt
        typedef hls::x_complex<ap_fixed<(W1 + 1) * 2, I1 + 1, AP_RND_CONV, AP_SAT, 0> > OFF_DIAG_T; // Takes result of /
        typedef ap_fixed<2 + (W2 - I2) + W2, 2 + (W2 - I2), AP_RND_CONV, AP_SAT, 0> RECIP_DIAG_T;
        typedef hls::x_complex<ap_fixed<W2, I2, AP_RND_CONV, AP_SAT, 0> >
            L_OUTPUT_T; // Takes new L value.  Same as L output but saturation set
        static const int ARCH = 1;
        static const int INNER_II = 1;
        static const int UNROLL_FACTOR = 1;
        static const int UNROLL_DIM = (LowerTriangularL == true ? 1 : 2);
        static const int ARCH2_ZERO_LOOP = true;
    };


If the input datatype is **std::complex<ap_fixed>**, the **choleskyTrais** is defined as bellow:

.. code::

    template <bool LowerTriangularL,
              int RowsColsA,
              int W1,
              int I1,
              ap_q_mode Q1,
              ap_o_mode O1,
              int N1,
              int W2,
              int I2,
              ap_q_mode Q2,
              ap_o_mode O2,
              int N2>
    struct choleskyTraits<LowerTriangularL,
                          RowsColsA,
                          std::complex<ap_fixed<W1, I1, Q1, O1, N1> >,
                          std::complex<ap_fixed<W2, I2, Q2, O2, N2> > > {
        typedef std::complex<ap_fixed<W1 + W1, I1 + I1, AP_RND_CONV, AP_SAT, 0> > PROD_T;
        typedef std::complex<ap_fixed<(W1 + W1) + BitWidth<RowsColsA>::Value,
                                      (I1 + I1) + BitWidth<RowsColsA>::Value,
                                      AP_RND_CONV,
                                      AP_SAT,
                                      0> >
            ACCUM_T;
        typedef std::complex<ap_fixed<W1 + 1, I1 + 1, AP_RND_CONV, AP_SAT, 0> > ADD_T;
        typedef std::complex<ap_fixed<(W1 + 1) * 2, I1 + 1, AP_RND_CONV, AP_SAT, 0> > DIAG_T;     // Takes result of sqrt
        typedef std::complex<ap_fixed<(W1 + 1) * 2, I1 + 1, AP_RND_CONV, AP_SAT, 0> > OFF_DIAG_T; // Takes result of /
        typedef ap_fixed<2 + (W2 - I2) + W2, 2 + (W2 - I2), AP_RND_CONV, AP_SAT, 0> RECIP_DIAG_T;
        typedef std::complex<ap_fixed<W2, I2, AP_RND_CONV, AP_SAT, 0> >
            L_OUTPUT_T; // Takes new L value.  Same as L output but saturation set
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

.. Warning::

    * The function assumes that the input matrix is symmetric positive definite (Hermitian positive definite for complex-valued inputs). 
    * If the input matrix data type is ap_fixed or complex<ap_fixed>, please give proper parameters to ensure the input matrix is symmetric positive definite/Hermitian positive definte.
    * The definition of ap_[u]fixed<W,I,Q,O,N>

       - W: the Word length in bits. 
       - I: the number of bits above the decimal point.
       - Q: Quantization mode.
       - O: Quantization mode.
       - N: This defines the number of saturation bits in overflow wrap modes.


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

      - 0: Uses the lowest DSP utilization and lowest throughput.
      - 1: Uses higher DSP utilization but minimized memory utilization with increased throughput. This value does not support inner loop unrolling to further increase throughput.
      - 2: Uses highest DSP and memory utilization. This value supports inner loop unrolling to improve overall throughput with a limited increase in DSP resources. This is the most flexible architecture for design exploration.
   * Inner loop pipeling

       - >1: For ARCH 2, enables resource share and reduces the DSP utilization. When using complex floating-point data types, setting the value to two or four significantly reduces DSP utilization.
   * Inner loop unrolling

       - For ARCH 2, duplicates the hardware required to implement the loop processing by a specified factor, executes the corresponding number of loop iterations in parallel, and increases throughput but also increases DSP and memory utilization.

