.. 
   
.. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Least Squares Solution using QR Decomposition
   :description: This function computes the Least Squares Solution using QR decomposition.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

***********************
Least Squares Solution using QR Decomposition 
***********************

Introduction
==============

The method of least squares is a parameter estimation method in regression analysis based on minimizing the sum of the squares of the residuals (a residual being the difference between an observed value and the fitted value provided by a model) made in the results of each individual equation.
given matrix :math:`A_\mathrm{mxn}`, vector :math:`\vec{b}_\mathrm{m}`, find vector :math:`\vec{x}_\mathrm{n}` that minimize

.. math::
    min(\|Ax - b\|)

Solution of the least squares problems: any :math:`\hat{x}` that satisfies:

.. math::
    \|A\hat{x} - b\| \leq \|A\vec{x} - b\|    

- :math:`\vec{x}` is for all :math:`\vec{x}` 
- :math:`\hat{x}` is the residual vector.
- :math:`\hat{r} = \vec{b} - A\vec{x}`
- if :math:`\hat{r} = 0`, the :math:`\hat{x}` solves the linear equation :math:`Ax=b`.
- if :math:`\hat{r} \neq 0`, the :math:`\hat{x}` is a least squares approximate solution of the equation.

In this design, we solve Least-squares problems via QR factorization. Given a matrix `A` , the goal is to find two matrixes `Q` and `R` such that `Q` is orthogonal and `R` is upper triangular. If `m≥n` , then

.. math::
    \underbrace{A}_{m \times n} =  \underbrace{Q}_{m \times m} \underbrace{\begin{bmatrix} R \\ 0 \end{bmatrix}}_{m \times n} = \begin{bmatrix} Q_1 & Q_2 \end{bmatrix} \underbrace{\begin{bmatrix} R \\ 0 \end{bmatrix}}_{m \times n}

.. math::
    \underbrace{A}_{m \times n} = \underbrace{Q_1}_{m \times n} \mbox{ } \underbrace{R}_{n \times n}

.. math::
    \hat{x} = R^{-1} \times Q_1^T \times \vec{b}
    

Entry Point 
==============

The graph entry point is the following:

.. code::
    xf::solver::LSTQR_Graph

Template Parameters
---------------------
* `rowA_num`: the number of rows for input matrix `A`;
* `colA_num`: the number of columns for input matrix `A`;
* `rowB_num`: the dim of vector `B`;
* `colB_num`: the number of vector `B`, more than one vectors is supported;

To access more details, see :ref:`AIE APIs Overview`.

Ports
-------
To access more details, see :ref:`AIE APIs Overview`.


AIE Graph
===============

Design Notes
--------------------
* Target: :math:`A\vec{x}\thickapprox\vec{b}`, :math:`A_{M \times N}` is input matrix, :math:`\vec{b}` is input vector, :math:`\hat{x}` is the least squares solution. 
* DataType supported: `cfloat`.
* DataSize supported: input matrix size :math:`M` is the times of 4 and no bigger than 1024, and :math:`N` shoulb be no bigger than 256.
* Description: 
    * There are three parts, the first part is matrix QR decomposition with householder method, the second part is transform data to the formate which will be used by back-substitution. The third part is back-substitution.   
    * For the first part: 

        * AIE core function calculate the reflection vector in column k, and then update the rest of elements of :math:`matA` and calculate orthogonal matrix :math:`matQ \times \vec{b}`. 
        * As reflection vector is calculated column by column, :math:`N` AIE cores are used, :math:`N` is the column number of input matrix. The previous core's output is fed to the next core's input, and on and on, till the last column is computed;
        * Matrix A and vector :math:`\vec{b}` are concated, the fist `M*N` datas are matrix A, the rest of `M` datas are vector :math:`\vec{b}`;
        * Matrix R and vector :math:`\vec{c}` are concated, the first `M*N` datas are matrix R, the rest `M` datas are vector :math:`\vec{c}` which is calculated by :math:`\vec{c}=Q^T \times \vec{b}`. 
        * Each column of inputs are injected to two input stream in such way: Elem[N*4] and Elem[N*4+1] to stream 0, Elem[N*4+2], Elem[N*4+3] to stream 1.
        * Each column of outputs are extracted in the same way as inputs, but from output stream 0 and 1.

    * For the second part, targeted to data transformation:

        * The input data is from the output of the first part. The output is fed to the third part.
        * The inputs are injected to two input stream in such way: Elem[N*4] and Elem[N*4+1] to stream 0, Elem[N*4+2], Elem[N*4+3] to stream 1.
        * The outputs are injected to two input stream in such way: the real part of cfloat data to stream 0, the imag part of cfloat data to stream 1.
        * The input triangular matrix data is from the first column to last column, include the whole data, data type is cfloat. 
        * The output triangular matrix data is from the last column to first colum, from the last element to first element for each column vector. complex data is concated by the real part and imag part in two streams. Besides, the output triangular matrix only store valid datas, that is zeros datas below diagonal are not stored.

    * The third part:

        * Target is to solve :math:`x=A^{-1} \times b`. 
        * Each AIE core function deals with one column datas.   
        * This design takes two streams as input interface and two streams as output interface.
        * It takes input of triangular matrix R and vector :math:`\vec{c}` as inputs, and generate output vector :math:`\vec{x}`.
        * Triangular Matrix R and vector :math:`\vec{c}` are concated. First triangular matrix column by column, then vector :math:`\vec{c}`. 
        * inputs are injected to two input stream in such way: real part of complex data to stream 0, imag part of complex data to stream 1.
        * outputs are injected to two output stream in such way: real part of complex data to stream 0, imag part of complex data to stream 1.

Graph Interfaces
--------------------

.. code::

   template <int M, int N, int K>
   void lstqr(input_stream<cfloat>* __restrict matAB_0,
              input_stream<cfloat>* __restrict matAB_1,
              output_stream<cfloat>* __restrict matRC_0,
              output_stream<cfloat>* __restrict matRC_1,
              const int column_id);

.. note::

   * To utilize bandwidth of input / output stream, the input matrix and output result are transfered in such way: Elem[N*4] and Elem[N*4+1] are transferred with matAB_0/matRC_0, Elem[N*4+2] and Elem[N*4+3] are transferred with matAB_1/matRC_1. 


* Input:

  *  ``input_stream<cfloat>* matAB_0``    stream of input matrix, contains lower two elements of each 4 elements.
  *  ``input_stream<cfloat>* matAB_1``    stream of input matrix, contains higher two elements of each 4 elements.
  *  ``column_id``                        column id. 

* Output:

  *  ``output_stream<cfloat>* matRC_0``    stream of output matrix, contains lower two elements of each 4 elements.
  *  ``output_stream<cfloat>* matRC_1``    stream of output matrix, contains higher two elements of each 4 elements.

.. code::

   template <int M, int N, int K>
   void backSubstitution(input_stream<float>* __restrict matRC_0,
                         input_stream<float>* __restrict matRC_1,
                         output_stream<float>* __restrict matXC_0,
                         output_stream<float>* __restrict matXC_1,
                         const int column_id);

.. Note::

   * To utilize the bandwidth of input / output stream, the input matrix and output result are divided into real part and imag part, matRC_0/matXC_0 containes the real part datas and matRC_1/matXC_1 contains the image part datas. 


* Input:

  *  ``input_stream<float>* matRC_0``    stream of input matrix, contains the real part datas. 
  *  ``input_stream<float>* matRC_1``    stream of input matrix, contains the imag part datas.
  *  ``column_id``                        column id. 

* Output:

  *  ``output_stream<float>* matXC_0``    stream of output matrix, contains the real part datas.
  *  ``output_stream<float>* matXC_1``    stream of output matrix, contains the imag part datas.
