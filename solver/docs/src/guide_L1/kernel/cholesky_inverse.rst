..
   .. Copyright © 2021–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_..

.. meta::
   :keywords: Cholesky_Inverse
   :description: Matrix inverse with usage of Cholesky Decomposition
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

*******************************************************
Cholesky_Inverse 
*******************************************************

Overview
============
Cholesky_Inverse, matrix inversion with the usage of Cholesky decomposition.  

Cholesky decomposition is a decomposition of a Hermitian, positive-definite matrix into the product of a lower triangular matrix and its conjugate transpose, in the form of :math:`A = LL^*`. :math:`A` is a Hermitian positive-definite matrix, :math:`L` is a lower triangular matrix with real and positive diagonal entries, and :math:`L^*` denotes the conjugate transpose of :math:`L`. 
Cholesky decomposition is useful for efficient numerical solutions. 

matrix :math:`A^{-1}` could be computed in the form of :math:`A^{-1} = (L^{-1})^*L^{-1}`. 

.. math::

            AA^{-1} = I

            A = L*L^* 

            A^{-1} = (L^{-1})^*L^{-1}

As matrix :math:`L` is a triangular matrix, :math:`L^{-1}` is easy to compute. 

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

.. note::
   * The function assumes that the input matrix is symmetric positive definite (Hermitian positive definite for complex-valued inputs).
   * For floating point types, subnormal input values are not supported. If used, the synthesized hardware flushes these to zero, and the behavior differs versus software simulation.

Interfaces
--------------------
* Template parameters:

    -  RowsColsA              Defines the matrix dimensions
    -  InputType              Input data type
    -  OutputType             Output data type
    -  CholeskyInverseTraits  Traits class
   
* Arguments:

    -  matrixAStrm             Stream of Square Hermitian/symmetric positive definite input matrix
    -  matrixInverseAStrm      Stream of Inverse of input matrix
    -  cholesky_success        Indicates if matrix A was successfully inverted. 0 = Success. 1 = Failure. 


Implementation Controls
------------------------

Specifications
~~~~~~~~~~~~~~~~~~~~~~~~~
* To implement choleskyInverse, the following sub-functions executed sequentially: Cholesky, back substitution, and matrix multiply. You can specify the individual sub-function implementations using a configuration class derived from the following basic class by redefining the appropriate class member: 

.. code::

    typedef xf::solver::choleskyInverseTraits<ROWSCOLSA, MATRIX_IN_T, MATRIX_OUT_T> DEFAULT_CHOL_INV_TRAITS;
    
    struct my_cholesky_inv_traits : DEFAULT_CHOL_INV_TRAITS {
        struct BACK_SUBSTITUTE_TRAITS : xf::solver::backSubstituteTraits<ROWSCOLSA,
                                                                         DEFAULT_CHOL_INV_TRAITS::CHOLESKY_OUT,
                                                                         DEFAULT_CHOL_INV_TRAITS::BACK_SUBSTITUTE_OUT> {
            static const int ARCH = SEL_ARCH;
        };
    };

The configuration class is supplied to the **xf::solver::choleskyInverse** function as a template parameter as follows.

.. code::

    xf::solver::choleskyInverse<ROWSCOLSA, MATRIX_IN_T, MATRIX_OUT_T, my_cholesky_inv_traits>(
        matrixAStrm, matrixInverseAStrm, inverse_OK);


The definition of **xf::solver::choleskyInverseTraits** is in the file **L1/include/hw/cholesky_inverse.hpp** .

The default **xf::solver::choleskyInverseTraits** struct defining the internal variable types for the cholesky inverse function, showed as follows:

.. code::

   template <int RowsColsA, typename InputType, typename OutputType>
   struct choleskyInverseTraits {
       typedef InputType CHOLESKY_OUT;
       typedef choleskyTraits<false, RowsColsA, InputType, InputType> CHOLESKY_TRAITS;
       typedef InputType BACK_SUBSTITUTE_OUT;
       typedef backSubstituteTraits<RowsColsA, InputType, InputType> BACK_SUBSTITUTE_TRAITS;
       typedef matrixMultiplyTraits<NoTranspose,
                                    ConjugateTranspose,
                                    RowsColsA,
                                    RowsColsA,
                                    RowsColsA,
                                    RowsColsA,
                                    InputType,
                                    OutputType>
           MATRIX_MULTIPLY_TRAITS;
   };


If the input datatype is ap_fixed, the **xf::solver::choleskyInverseTraits** struct is defined as follows:

.. code::

   template <int RowsColsA,
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
   struct choleskyInverseTraits<RowsColsA, ap_fixed<W1, I1, Q1, O1, N1>, ap_fixed<W2, I2, Q2, O2, N2> > {
       // Cholesky decomposition output precision
       static const int CholeskyOutputW = W1;
       static const int CholeskyOutputI = I1;
       static const ap_q_mode CholeskyOutputQ = Q1;
       static const ap_o_mode CholeskyOutputO = O1;
       static const int CholeskyOutputN = N1;
       typedef ap_fixed<CholeskyOutputW, CholeskyOutputI, CholeskyOutputQ, CholeskyOutputO, CholeskyOutputN> CHOLESKY_OUT;
       typedef choleskyTraits<false, RowsColsA, ap_fixed<W1, I1, Q1, O1, N1>, CHOLESKY_OUT> CHOLESKY_TRAITS;
       // Back substitution output precision
       static const int BackSubstitutionOutW = W2;
       static const int BackSubstitutionOutI = I2;
       static const ap_q_mode BackSubstitutionOutQ = Q2;
       static const ap_o_mode BackSubstitutionOutO = O2;
       static const int BackSubstitutionOutN = N2;
       typedef ap_fixed<BackSubstitutionOutW,
                        BackSubstitutionOutI,
                        BackSubstitutionOutQ,
                        BackSubstitutionOutO,
                        BackSubstitutionOutN>
           BACK_SUBSTITUTE_OUT;
       typedef backSubstituteTraits<RowsColsA, CHOLESKY_OUT, BACK_SUBSTITUTE_OUT> BACK_SUBSTITUTE_TRAITS;
       typedef matrixMultiplyTraits<NoTranspose,
                                    ConjugateTranspose,
                                    RowsColsA,
                                    RowsColsA,
                                    RowsColsA,
                                    RowsColsA,
                                    BACK_SUBSTITUTE_OUT,
                                    ap_fixed<W2, I2, Q2, O2, N2> >
           MATRIX_MULTIPLY_TRAITS;
   };

If the input datatype is hls::compelx<ap_fixed>, the **xf::solver::choleskyInverseTraits** struct is defined as follows:

.. code::

   template <int RowsColsA,
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
   struct choleskyInverseTraits<RowsColsA,
                                hls::x_complex<ap_fixed<W1, I1, Q1, O1, N1> >,
                                hls::x_complex<ap_fixed<W2, I2, Q2, O2, N2> > > {
       // Cholesky decomposition output precision
       static const int CholeskyOutputW = W1;
       static const int CholeskyOutputI = I1;
       static const ap_q_mode CholeskyOutputQ = Q1;
       static const ap_o_mode CholeskyOutputO = O1;
       static const int CholeskyOutputN = N1;
       typedef hls::x_complex<
           ap_fixed<CholeskyOutputW, CholeskyOutputI, CholeskyOutputQ, CholeskyOutputO, CholeskyOutputN> >
           CHOLESKY_OUT;
       typedef choleskyTraits<false, RowsColsA, hls::x_complex<ap_fixed<W1, I1, Q1, O1, N1> >, CHOLESKY_OUT>
           CHOLESKY_TRAITS;
       // Back substitution output precision
       static const int BackSubstitutionOutW = W2;
       static const int BackSubstitutionOutI = I2;
       static const ap_q_mode BackSubstitutionOutQ = Q2;
       static const ap_o_mode BackSubstitutionOutO = O2;
       static const int BackSubstitutionOutN = N2;
       typedef hls::x_complex<ap_fixed<BackSubstitutionOutW,
                                       BackSubstitutionOutI,
                                       BackSubstitutionOutQ,
                                       BackSubstitutionOutO,
                                       BackSubstitutionOutN> >
           BACK_SUBSTITUTE_OUT;
       typedef backSubstituteTraits<RowsColsA, CHOLESKY_OUT, BACK_SUBSTITUTE_OUT> BACK_SUBSTITUTE_TRAITS;
       typedef matrixMultiplyTraits<NoTranspose,
                                    ConjugateTranspose,
                                    RowsColsA,
                                    RowsColsA,
                                    RowsColsA,
                                    RowsColsA,
                                    BACK_SUBSTITUTE_OUT,
                                    hls::x_complex<ap_fixed<W2, I2, Q2, O2, N2> > >
           MATRIX_MULTIPLY_TRAITS;
   };
   
If the input datatype is std::compelx<ap_fixed>, the **xf::solver::choleskyInverseTraits** struct is defined as follows:

.. code:: 

   template <int RowsColsA,
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
   struct choleskyInverseTraits<RowsColsA,
                                std::complex<ap_fixed<W1, I1, Q1, O1, N1> >,
                                std::complex<ap_fixed<W2, I2, Q2, O2, N2> > > {
       // Cholesky decomposition output precision
       static const int CholeskyOutputW = W1;
       static const int CholeskyOutputI = I1;
       static const ap_q_mode CholeskyOutputQ = Q1;
       static const ap_o_mode CholeskyOutputO = O1;
       static const int CholeskyOutputN = N1;
       typedef std::complex<ap_fixed<CholeskyOutputW, CholeskyOutputI, CholeskyOutputQ, CholeskyOutputO, CholeskyOutputN> >
           CHOLESKY_OUT;
       typedef choleskyTraits<false, RowsColsA, std::complex<ap_fixed<W1, I1, Q1, O1, N1> >, CHOLESKY_OUT> CHOLESKY_TRAITS;
       // Back substitution output precision
       static const int BackSubstitutionOutW = W2;
       static const int BackSubstitutionOutI = I2;
       static const ap_q_mode BackSubstitutionOutQ = Q2;
       static const ap_o_mode BackSubstitutionOutO = O2;
       static const int BackSubstitutionOutN = N2;
       typedef std::complex<ap_fixed<BackSubstitutionOutW,
                                     BackSubstitutionOutI,
                                     BackSubstitutionOutQ,
                                     BackSubstitutionOutO,
                                     BackSubstitutionOutN> >
           BACK_SUBSTITUTE_OUT;
       typedef backSubstituteTraits<RowsColsA, CHOLESKY_OUT, BACK_SUBSTITUTE_OUT> BACK_SUBSTITUTE_TRAITS;
       typedef matrixMultiplyTraits<NoTranspose,
                                    ConjugateTranspose,
                                    RowsColsA,
                                    RowsColsA,
                                    RowsColsA,
                                    RowsColsA,
                                    BACK_SUBSTITUTE_OUT,
                                    std::complex<ap_fixed<W2, I2, Q2, O2, N2> > >
           MATRIX_MULTIPLY_TRAITS;
   };


.. Warning::

    * The function assumes that the input matrix is symmetric positive definite (Hermitian positive definite for complex-valued inputs). 
    * If the input matrix data type is ap_fixed or complex<ap_fixed>, please give proper parameters to ensure the input matrix is symmetric positive definite/Hermitian positive definte.
    * The definition of ap_[u]fixed<W,I,Q,O,N>

       - W: the Word length in bits. 
       - I: the number of bits above the decimal point.
       - Q: Quantization mode.
       - O: Quantization mode.
       - N: This defines the number of saturation bits in overflow wrap modes.



