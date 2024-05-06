.. 
   
.. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: QR Decomposition
   :description: This function computes the QR decomposition of matrix.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

***********************
QR Decomposition 
***********************

Introduction
==============

This function computes the QR decomposition of matrix :math:`A`

.. math::
    A = Q R

where :math:`A` is a matrix of size :math:`m \times n`, :math:`Q` is orthogonal matrix of size :math:`m \times m`, and :math:`R` is an upper triangular matrix. 
If instead A is a complex square matrix, then there is a decomposition A = QR where Q is a unitary matrix.


Entry Point 
==============

The graph entry point is the following:

.. code::
    xf::solver::internal::QRDComplexFloat

Template Parameters
---------------------
* `column_num`: the number of columns;
* `row_num`: the number of rows;
* `k_rep`: the number of input matrixs that are processed by one graph call.

To access more details, see :ref:`AIE APIs Overview`.

Ports
-------
To access more details, see :ref:`AIE APIs Overview`.


AIE Graph
===============

Design Notes
--------------------
* Target: :math:`A=QR`, :math:`A[M*N]` is input matrix, :math:`Q[M*N]` and  :math:`R[N*N]` are the output matrix via QR decomposition. 
* DataType supported: `cfloat`.
* DataSize supported: input matrix size :math:`M` is the times of 4 and no bigger than 1024, and :math:`N` shoulb be no bigger than 256.
* Description: 
    * The AIE core function calculate one column of :math:`Q` and update the rest elements of :math:`matA` 
    * To calculate all columns of :math:`Q`, N AIE cores are used, N is the number of input matrix's column number. The first core's output is fed to the second core's input, and continued till the last column is computed;
    * The information of matrix dimension, the target column id which is updated, and so on are read from the head of input streams;
* Implementation Notes:
    * This design utilzied "Modified Gram-Schmidt" method to solve QR decomposition.
    * This design takes two streams as input interface and two streams as output interface.
    * It takes input of matrix A and matrix I (identity matrix) as inputs, and generate matrix Q and R as output.
    * Matrix A and I are concated in row to form a (m+n) rows x (n) columns matrix, so Elements[0:M-1] of each column are from A and Elements[M:M+N-1] are from I.
    * Matrix Q and R are concated in row to form a (m+n) rows x (n) columns matrix, so Elements[0:M-1] of each column are from Q and Elements[M:M+N-1] are from R.
    * Concated inputs are injected column by column, concated outputs are extracted in the same way.
    * Each column of inputs are injected to two input stream in such way: Elem[N*4] and Elem[N*4+1] to stream 0, Elem[N*4+2], Elem[N*4+3] to stream 1.
    * Each column of outputs are extracted in the same way as inputs, but from output stream 0 and 1.

Graph Interfaces
--------------------

.. Code::

   void GramSchmidtKernelComplexFloat::process(input_stream_cfloat* in_0,
                                               input_stream_cfloat* in_1,
                                               output_stream_cfloat* out_0,
                                               output_stream_cfloat* out_1);

.. Note::

   * To utilize the bandwidth of input / output stream, the input matrix and output result are transferred in such way: Elem[N*4] and Elem[N*4+1] are transferred with in_0 / out_0, Elem[N*4 + 2] and Elem[N*4 + 3] are transferred with in_1 / out_1.


* Input:

  *  ``input_stream_cfloat* in_0``    stream of input matrix, contains lower two elements of each 4 elements.
  *  ``input_stream_cfloat* in_1``    stream of input matrix, contains higher two elements of each 4 elements.

* Output:

  *  ``input_stream_cfloat* out_0``    stream of output matrix, contains lower two elements of each 4 elements.
  *  ``input_stream_cfloat* out_1``    stream of output matrix, contains higher two elements of each 4 elements.