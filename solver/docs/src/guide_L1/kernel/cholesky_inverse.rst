..
   Copyright 2021 Xilinx, Inc.
  
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
  
       http://www.apache.org/licenses/LICENSE-2.0
  
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

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
   Subnormall values are not supported. If used, the synthesized hardware will flush these to zero, and the behavior will differ versus software simulation.

Interfaces
--------------------
* Template parameters:

  *  RowsColsA              Defines the matrix dimensions
  *  InputType              Input data type
  *  OutputType             Output data type
  *  CholeskyInverseTraits  Traits class
   
* Arguments:

  * matrixAStrm             Stream of Square Hermitian/symmetric positive definite input matrix
  * matrixInverseAStrm      Stream of Inverse of input matrix
  * cholesky_success        Indicates if matrix A was successfully inverted. 0 = Success. 1 = Failure.


Implementation Controls
------------------------

Specifications
~~~~~~~~~~~~~~~~~~~~~~~~~
The DATAFLOW directive is applied to the top function. User could specify the individual sub-function implementiontations using a configuration class derived from the following basic class by redefining the appropriate class member: 

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

The configuration class is supplied to the **xf::solver::choleskyInverse** function as a template paramter as follows.
The sub-functions are executed sequentially: Cholesky, back substitution, and matrix multiply. The implementation selected for these sub-functions determines the resource utilization and function throughput/latency of the Inverse function.

.. code::

   template <int RowsColsA,
             typename InputType,
             typename OutputType,
             typename CholeskyInverseTraits = choleskyInverseTraits<RowsColsA, InputType, OutputType> >
   void choleskyInverse(hls::stream<InputType>& matrixAStrm,
                         hls::stream<OutputType>& matrixInverseAStrm,
                         int& cholesky_success) {
   #pragma HLS DATAFLOW
       hls::stream<typename CholeskyInverseTraits::CHOLESKY_OUT> matrixUStrm;
   #pragma HLS STREAM variable = matrixUStrm depth = 16
       hls::stream<typename CholeskyInverseTraits::BACK_SUBSTITUTE_OUT> matrixInverseUStrm;
   #pragma HLS STREAM variable = matrixInverseUStrm depth = 16
       int U_singular;
   
       // Run Cholesky, get upper-triangular result
       const bool LOWER_TRIANGULAR = false;
       cholesky_success = cholesky<LOWER_TRIANGULAR, RowsColsA, InputType, typename CholeskyInverseTraits::CHOLESKY_OUT, typename CholeskyInverseTraits::CHOLESKY_TRAITS>(matrixAStrm, matrixUStrm);
   
       // Run back-substitution to compute U^-1
       backSubstitute<RowsColsA, typename CholeskyInverseTraits::CHOLESKY_OUT, typename CholeskyInverseTraits::BACK_SUBSTITUTE_OUT, typename CholeskyInverseTraits::BACK_SUBSTITUTE_TRAITS>(matrixUStrm, matrixInverseUStrm, U_singular);

       // A^-1 = U^-1*U^-t (equivalent to L-t*L-1)
       matrixMultiply<NoTranspose, ConjugateTranspose, RowsColsA, RowsColsA, RowsColsA, RowsColsA, typename CholeskyInverseTraits::BACK_SUBSTITUTE_OUT, OutputType, typename CholeskyInverseTraits::MATRIX_MULTIPLY_TRAITS>(matrixInverseUStrm, matrixInverseAStrm);
   }


