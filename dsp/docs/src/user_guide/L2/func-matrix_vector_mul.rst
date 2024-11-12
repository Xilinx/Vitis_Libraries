..
   Copyright © 2019–2024 Advanced Micro Devices, Inc
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _MATRIX_VECTOR_MULTIPLY:

======================
Matrix-Vector Multiply
======================

The DSPLib contains a Matrix-Vector Multiply/GEMV (General Matrix Vector Multiplication) solution. The GEMV has two input ports, one for each input operand. Only IO-buffer connections are supported.

The input IO-buffer for the matrix is defined as Matrix A (inA) and is described by the ``TP_DIM_A`` and ``TP_DIM_B`` template parameters which specify the number of rows and columns in the matrix, respectively.

The second IO-buffer of data is defined as Vector B (inB) and will be a vector with a size of ``TP_DIM_B`` elements. The number of columns and the number of elements in the vector must be equal and are therefore defined by the same template parameter.

The output IO-buffer containing the result of the matrix-vector multiplication is connected to the output port. The output data will be a vector of size ``TP_DIM_A``.

Entry Point
===========

The graph entry point is the following:

.. code-block::

    xf::dsp::aie::blas::matrix_vector_mul::matrix_vector_mul_graph

Device Support
==============
The Matrix-Vector Multiply supports AIE and AIE-ML devices.

Supported Types
===============

The Matrix-Vector Multiply supports matrices of an integer type (int16, cint16, int32, or cint32) multiplied by a vector of an integer type. Matrices containing elements with a floating-point type (float or cfloat) are supported but must be multiplied with a vector of floats. Multiplication of an integer type with a floating-point type is not supported.

Template Parameters
===================

To see details on the template parameters for the Matrix Vector Multiply, see :ref:`API_REFERENCE`.

Access Functions
================

To see details on the access functions for the Matrix-Vector Multiply, see :ref:`API_REFERENCE`.

Ports
=====

To see details on the ports for the Matrix-Vector Multiply, see :ref:`API_REFERENCE`.

Design Notes
============

The matrix-vector multiplication kernel requires the matrix data in a column-major format where the columns are stored contiguously in memory and ``TP_DIM_A_LEADING = 1``.
Setting ``TP_DIM_A_LEADING`` to 0, indicates to the graph that the data is in a row-major format and DMA buffer descriptors will be to set the write access of the kernel input so that a transpose can be achieved. This feature is currently only compatible with int32, cint16 and float data types for a single frame per buffer ``NUM_FRAMES = 1``. If multiple frames are required or the data type is int16, cint32 or cfloat, the matrix data must be transposed to a column-major format with the template parameter ``TP_DIM_A_LEADING`` being set to 1.

Maximum matrix dimensions per kernel
------------------------------------

The maximum memory accessible by a kernel is 32 kB for AIE and 64kB for AIE-ML. The maximum matrix dimensions per kernel are limited by the memory requirements and how much memory is available.

A matrix_vector_mul design needs to allocate memory for the following:

* IO-buffer Size A: Input matrix A of size ``(TP_DIM_A / TP_SSR) x (TP_DIM_B / TP_CASC_LEN) x sizeof(TT_DATA_A)``.

* IO-buffer Size B: Input vector B of size ``(TP_DIM_B / TP_CASC_LEN) x sizeof(TT_DATA_B)``.

* IO-buffer Size Out: Output vector of size ``(TP_DIM_A / TP_SSR) x sizeof(TT_DATA_OUT)``.

Furthermore, if these buffers are ping-pong buffers, their memory requirement doubles in size. This can be reduced by using the single_buffer constraint on the buffer.

The cascading and SSR feature of the Matrix-Vector Multiply can be used if the size of the matrix and vector exceeds the maximum memory of a single kernel. This works as the matrix and vector data will be split across multiple kernels resulting in a reduced per-kernel memory usage.

Cascaded Kernels
----------------

Multiple kernels can be chained together in cascade using the ``TP_CASC_LEN`` template parameter. The input matrix and vector will be split across ``TP_DIM_B`` and processed to a ``TP_CASC_LEN`` number of kernels. The accumulated partial results of each kernel are passed to the successive kernel via a cascade stream until the end of the cascade chain, whereby the final kernel will output the expected results to the output port. Cascade connections are made internally to the matrix multiply graph and external interfaces to the graph remain unchanged.

Each AI Engine kernel in the array is given a sub-matrix and a split of the vector, so the interface to the graph is an array of ports for both A and B. The split will occur along the ``TP_DIM_B`` dimension. For example, the matrix data to each kernel will be of a size ``TP_DIM_A`` x ``TP_DIM_B/TP_CASC_LEN``, and the vector will contain ``TP_DIM_B/TP_CASC_LEN`` elements.

SSR
---
Multiple cascaded kernel chains can be used in parallel using the ``TP_SSR`` template parameter. The input matrix will be split across the ``TP_DIM_A`` dimension for each rank of cascade, but there will be no split for the input vector which is only split when ``TP_CASC_LEN > 1``. Each rank of SSR will produce an equal split of the output. The outputs to each SSR rank should be concatenated together to produce the resulting final output of the matrix-vector multiplication.

The number of rows in the matrix (``TP_DIM_A``) must be a multiple of ``256/ 8/sizeof(TT_DATA_A)``. This is equivalent to the number of samples of ``TT_DATA_A`` that can occupy a 256-bit register. When SSR is being used, the value of ``TP_DIM_A`` must also be a multiple of ``TP_SSR``.

The number of columns and size of the input vector (``TP_DIM_B``) must be a multiple of ``256/sizeof(TT_DATA_B)``. When multiple kernels are used in the cascade, the value of ``TP_DIM_B`` must also be a multiple of ``TP_CASC_LEN``.

Matrix and vector input data can be zero-padded to meet these requirements.

You can find a full list of descriptions and parameters in :ref:`API_REFERENCE`.

Connections to the cascade and ssr ports can be made as follows:

.. code-block::

    for (int ssrIdx = 0; ssrIdx < TP_SSR; ssrIdx++) {
        for (int cascIdx = 0 ; cascIdx < TP_CASC_LEN; cascIdx++) {
            connect<>(inA[(ssrIdx * TP_CASC_LEN) + cascIdx], matrix_vector_mulGraph.inA[(ssrIdx * TP_CASC_LEN) + cascIdx]);
            connect<>(inB[(ssrIdx * TP_CASC_LEN) + cascIdx], matrix_vector_mulGraph.inB[(ssrIdx * TP_CASC_LEN) + cascIdx]);
        }
        connect<>(matrix_vector_mulGraph.out[ssrIdx], out[ssrIdx]);
    }

Constraints
-----------

In the entry level graph, the following names are used to identify the various kernels as follows:

'm_mat_vec_mulKernels': This is an array of kernel pointers returned by getKernels which point to the kernels in the SSR and cascade array. These are the kernels perform the matrix-vector multiply operations.

The index of the kernel increments along the cascade chain first.
For example, a GEMV design with an ``TP_SSR > 1`` and ``TP_CASC_LEN > 1`` where ssrIdx specifies the rank of SSR and cascIdx specifies the position of the kernel along the cascade chain, the index of the kernel can be found by ``(ssrIdx * TP_CASC_LEN) + cascIdx``.


Code Example
============

The following code example shows how the matrix_vector_mul_graph class can be used within a user super-graph. This example shows a 32x16 cint16 matrix multiplied by a cint16 vector of length 16.

.. literalinclude:: ../../../../L2/examples/docs_examples/test_matvec.hpp
    :language: cpp
    :lines: 17-


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