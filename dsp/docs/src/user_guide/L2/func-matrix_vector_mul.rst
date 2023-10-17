..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
    
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
    
       http://www.apache.org/licenses/LICENSE-2.0
    
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

.. _MATRIX_VECTOR_MULTIPLY:

======================
Matrix-Vector Multiply
======================

The DSPLib contains a Matrix-Vector Multiply/GEMV (General Matrix Vector Multiplication) solution. The GEMV has two input ports, one for each input operand. Only iobuffer connections are supported.

The input iobuffer for the matrix is defined as Matrix A (inA) and is described by the template parameters TP_DIM_A and TP_DIM_B which specify the number of rows and columns in the matrix, respectively.

The second iobuffer of data is defined as Vector B (inB) and will be a vector with a size of TP_DIM_B elements. The number of columns and the number of elements in the vector must be equal and are therefore defined by the same template parameter.

The output iobuffer containing the result of the matrix-vector multiplication is connected to the output port. The output data will be a vector of size TP_DIM_A.

~~~~~~~~~~~
Entry Point
~~~~~~~~~~~

The graph entry point is the following:

.. code-block::

    xf::dsp::aie::blas::matrix_vector_mul::matrix_vector_mul_graph

~~~~~~~~~~~~~~~
Supported Types
~~~~~~~~~~~~~~~

The Matrix-Vector Multiply supports matrices of an integer type (int16, cint16, int32, or cint32) multiplied by a vector of an integer type. Matrices containing elements with a floating-point type (float or cfloat) are supported but must be multiplied with a vector of floats. Multiplication of an integer type with a floating-point type is not supported.

~~~~~~~~~~~~~~~~~~~
Template Parameters
~~~~~~~~~~~~~~~~~~~

To see details on the template parameters for the Matrix Vector Multiply, see :ref:`API_REFERENCE`.

NOTE: Maximum matrix dimensions per kernel.
Maximum memory accessible by a kernel is 32kB for AIE. The maximum matrix dimensions per kernel is limited by the memory requirements and how much memory is available.

A matrix_vector_mul design needs to allocate memory for the following:

* iobuffer Size A =  Input matrix A of size TP_DIM_A x TP_DIM_B x sizeof(TT_DATA_A).

* iobuffer Size B =  Input vector B of size TP_DIM_B x sizeof(TT_DATA_B).

* iobuffer Size Out = Output vector of size TP_DIM_A x sizeof(TT_DATA_OUT).

Furthermore, if these buffers are ping-pong buffers, their memory requirement doubles in size. This can be reduced by using the single_buffer constraint on the buffer.

The cascading feature of the Matrix-Vector Multiply can be used if the size of the matrix and vector exceeds the maximum memory of a single kernel. This works as the matrix and vector data will be split across multiple kernels resulting in a reduced per-kernel memory usage.

~~~~~~~~~~~~~~~~
Access functions
~~~~~~~~~~~~~~~~

To see details on the access functions for the Matrix-Vector Multiply, see :ref:`API_REFERENCE`.

~~~~~
Ports
~~~~~

To see details on the ports for the Matrix-Vector Multiply, see :ref:`API_REFERENCE`.

~~~~~~~~~~~~
Design Notes
~~~~~~~~~~~~

The matrix-vector multiplication expects the matrix data of Matrix A (inA) to be stored in a column-major format. The data in each column must be stored contiguously in memory. A transpose of the matrix is required if the data is to be stored in a row major format.

Cascaded Kernels
----------------

Multiple kernels are chained together in cascade using the template parameter TP_CASC_LEN. The input matrix and vector will be split across TP_DIM_B and processed to a TP_CASC_LEN number of kernels. The accumulated partial results of each kernel are passed to the successive kernel via a cascade stream until the end of the cascade chain whereby the final kernel will output the expected results to the output port. Cascade connections are made internally to the matrix multiply graph and external interfaces to the graph remain unchanged.

Each AI Engine kernel in the array is given a sub-matrix and a split of the vector, so the interface to the graph is an array of ports for both A and B.  The split will occur along the TP_DIM_B dimension. For example, the matrix data to each kernel will be of a size TP_DIM_A x TP_DIM_B/TP_CASC_LEN and the vector will contain TP_DIM_B/TP_CASC_LEN elements.

The number of rows in the matrix (TP_DIM_A) must be a multiple of 256 / 8 / sizeof(TT_DATA_A), this is equivalent to the number of samples of TT_DATA_A that can occupy a 256-bit register.

The number of columns, and size of the input vector (TP_DIM_B) must be a multiple of 256 / sizeof(TT_DATA_B). When multiple kernels are used in cascade, the value of TP_DIM_B must also be a multiple of TP_CASC_LEN.

Matrix and vector input data can be zero-padded to meet these requirements.

Find a full list of descriptions and parameters in the :ref:`API_REFERENCE`.

Connections to the cascade ports can be made as follows:

.. code-block::

    for (int i = 0 ; i < TP_CASC_LEN; i++) {
        connect<>(inA[i], matrix_vector_mulGraph.inA[i]);
        connect<>(inB[i], matrix_vector_mulGraph.inB[i]);
    }
    connect<>( matrix_vector_mulGraph.out, out);

Constraints
-----------

In the entry level graph, the following names are used to identify the various kernels as follows:

'm_mat_vec_mulKernels’ - This is an array of kernel pointers returned by getKernels which point to the cascade kernels. These kernels perform the matrix-vector multiply operations.

~~~~~~~~~~~~
Code Example
~~~~~~~~~~~~

The following code example shows how the matrix_vector_mul_graph class may be used within a user super-graph. This example shows a 16x32 cint16 matrix multiplied by a cint16 vector of length 32.

.. literalinclude:: ../../../../L2/examples/docs_examples/test_matvec.hpp
    :language: cpp
    :lines: 17-58


.. |image1| image:: ./media/image1.png
.. |image2| image:: ./media/image2.png
.. |image3| image:: ./media/image4.png
.. |image4| image:: ./media/image2.png
.. |image6| image:: ./media/image2.png
.. |image7| image:: ./media/image5.png
.. |image8| image:: ./media/image6.png
.. |image9| image:: ./media/image7.png
.. |image10| image:: ./media/image2.png
.. |image11| image:: ./media/image2.png
.. |image12| image:: ./media/image2.png
.. |image13| image:: ./media/image2.png
.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:


