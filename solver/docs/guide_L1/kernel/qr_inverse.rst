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
   :keywords: QR_Inverse
   :description: matrix inverse with the usage of QR Factorization
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

******************************************************
QR_Inverse 
******************************************************

Overview
============
QR_Inverse, matrix inversion with the usage of QR decomposition.  As matrix :math:`A` could be decomposed into a product of an orthogonal matrix :math:`Q` and an upper triangular matrix :math:`R` in the form of :math:`A = QR`, the matrix inversion should be :math:`A^{-1} = R^{-1}Q^{-1} = R^{-1}Q^T`. 

.. math::

            AA^{-1} = I

            A = QR

            A^{-1} = R^{-1}Q^T

As matrix :math:`R` is an upper triangular matrix, :math:`R^{-1}` is easy to compute. In this design, :math:`R^{-1}` is computed via Backwark Subsititution.

Implementation
============

DataType Supported
--------------------
* float
* x_complex<float>
* std::complex<float>

.. note::
   Subnormall values are not supported. If used, the synthesized hardware will flush these to zero, and the behavior will differ versus software simulation.

Interfaces
--------------------
* Template parameters:

  *  RowsColsA:                 Defines the matrix dimensions
  *  InputType:                 Input data type
  *  OutputType:                Output data type
  *  QRInverseTraits:           QRInverse Traits class

* Arguments:

  * matrixAStrm:                Stream of Input matrix A
  * matrixInverseAStrm:         Stream of Inverse of input matrix
  * A_singular:                 1 = Failure, matrix A is singular; 0 = success

Implementation Controls
------------------------

Specifications
~~~~~~~~~~~~~~~~~~~~~~~~~
The DATAFLOW directive is applied to the top function. User could specify the individual sub-function implementiontations using a configuration class derived from the following basic class by redefining the appropriate class member: 

.. code::

   template <int RowsColsA, typename InputType, typename OutputType>
   struct qrInverseTraits {
       typedef float InternalType;
       typedef qrfTraits QRF_CONFIG;
       typedef backSubstituteTraits<RowsColsA, InternalType, InternalType> BACK_SUB_CONFIG;
       typedef matrixMultiplyTraits<NoTranspose,
                                    NoTranspose,
                                    RowsColsA,
                                    RowsColsA,
                                    RowsColsA,
                                    RowsColsA,
                                    InternalType,
                                    OutputType>
           MULTIPLIER_CONFIG;
   };


The configuration class is supplied to the **xf::solver::qrInverse** function as a template paramter as follows.
The sub-functions are executed sequentially: QRF, back substitution, and matrix multiply. The implementation selected for these sub-functions determines the resource utilization and function throughput/latency of the Inverse function.

.. code::

   template <int RowsColsA,
             typename InputType,
             typename OutputType,
             typename QRInverseTraits = qrInverseTraits<RowsColsA, InputType, OutputType> >
   void qrInverse(hls::stream<InputType>& matrixAStrm, hls::stream<OutputType>& matrixInverseAStrm, int& A_singular) {
   #pragma HLS DATAFLOW
       // Define intermediate buffers
       hls::stream<typename QRInverseTraits::InternalType> matrixQStrm;
   #pragma HLS STREAM variable = matrixQStrm depth = 16
       hls::stream<typename QRInverseTraits::InternalType> matrixRStrm;
   #pragma HLS STREAM variable = matrixRStrm depth = 16
       hls::stream<typename QRInverseTraits::InternalType> matrixInverseRStrm;
   #pragma HLS STREAM variable = matrixInverseRStrm depth = 16
   
       // Run QR factorization, get upper-triangular result in R, orthogonal/unitary matrix Q
       const bool TRANSPOSED_Q = true; // Q is produced in transpose form such that Q*A = R
       qrf<TRANSPOSED_Q, RowsColsA, RowsColsA, InputType, typename QRInverseTraits::InternalType, typename QRInverseTraits::QRF_CONFIG>(matrixAStrm, matrixQStrm, matrixRStrm);
   
        // Run back-substitution to compute R^-1
        backSubstitute<RowsColsA, typename QRInverseTraits::InternalType, typename QRInverseTraits::InternalType, typename QRInverseTraits::BACK_SUB_CONFIG>(matrixRStrm, matrixInverseRStrm, A_singular);
   
        // A^-1 = R^-1*Qt
        matrixMultiply<NoTranspose, NoTranspose, RowsColsA, RowsColsA, RowsColsA, RowsColsA, RowsColsA, RowsColsA, typename QRInverseTraits::InternalType, OutputType, typename QRInverseTraits::MULTIPLIER_CONFIG>(matrixInverseRStrm, matrixQStrm, matrixInverseAStrm);
    }


