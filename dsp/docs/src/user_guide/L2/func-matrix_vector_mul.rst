..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
   
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
The Matrix-Vector Multiply supports AIE, AIE-ML and AIE-MLv2 devices.

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

Reloadable Matrix Inputs via Run-Time Programmable (RTP) Ports
--------------------------------------------------------------
The template parameter ``TP_USE_MATRIX_RELOAD`` enables the matrix data to be supplied to each kernel through an RTP port, allowing the matrix data to be dynamically updated at runtime. When this parameter is set to 1, the following considerations apply:

**Matrix Splitting Across Kernels**:
- The matrix data must be split across the SSR ranks and cascade chain kernels. Each kernel receives a portion of the matrix based on the values of ``TP_SSR`` and ``TP_CASC_LEN``.
- For example, if ``TP_SSR = 2`` and ``TP_CASC_LEN = 3``, the matrix will be divided into ``2 * 3 = 6`` parts. The columns of the matrix will be split by ``TP_CASC_LEN`` and the rows by ``TP_SSR``. Each part is sent to a specific kernel via its corresponding RTP port.

**Matrix Format**:
- The matrix must be provided in a column-major format, where the elements of the columns are stored contiguously in memory. This ensures compatibility with the kernel's processing requirements.
- If the matrix is not in column-major format, it must be transposed before being supplied to the RTP ports.

**RTP Port Connections**:
- The RTP ports for the matrix are defined as ``matrixA`` in the graph. The number of RTP ports is equal to ``TP_SSR * TP_CASC_LEN``.
- Users must ensure that the correct portion of the matrix is sent to each RTP port. For example, the matrix data for the kernel at index ``(ssrIdx * TP_CASC_LEN) + cascIdx`` should be sent to ``matrixA[(ssrIdx * TP_CASC_LEN) + cascIdx]``.

**Runtime Updates**:
- The matrix data can be updated at runtime using the graph's `update()` method. This method takes the new matrix data as an argument and updates the corresponding RTP ports.
- The `update()` method must be called after the graph has been initialized but before the kernels start processing data.

**Interface Type for Vector B and Output**:
- The ``TP_API`` parameter can be used to select the interface type for the vector B input and the output. Setting ``TP_API = 1`` enables a streaming interface, while ``TP_API = 0`` uses an IO-buffer interface for vector B input and output.

**Dual Input and Multiple Outputs**:
- Additional parameters, ``TP_DUAL_IP`` and ``TP_NUM_OUTPUTS``, control the number of input and output ports when a streaming interface is used for vector B (``TP_API = 1``). If ``TP_DUAL_IP = 0``, each kernel receives a single vector B input. If ``TP_DUAL_IP = 1``, two stream inputs are provided (supported only for AIE devices). For ``TP_NUM_OUTPUTS``, a value of 1 results in a single stream output per SSR rank, while a value of 2 provides two stream outputs per SSR rank.

Note that ``TP_API``, ``TP_DUAL_IP``, and ``TP_NUM_OUTPUTS`` can only be used when ``TP_USE_MATRIX_RELOAD = 1``. Otherwise, their default values must be 0, 0, and 1, respectively.

By leveraging RTP ports, users can dynamically modify the matrix input data during runtime, enabling greater flexibility and adaptability in applications where the matrix values may change frequently. This approach is particularly useful in scenarios where the matrix dimensions and kernel configurations are designed to optimize memory usage and processing efficiency.


Leading matrix dimension
------------------------
It is recommended that the matrix data be in a column-major format where the elements of columns are stored contiguously in memory and ``TP_DIM_A_LEADING = 1``. This ensures optimal performance and compatibility with most configurations.

For example:

- In a column-major format, a 4x8 matrix:

    ::

            [1  5  9  13]
            [2  6  10 14]
            [3  7  11 15]
            [4  8  12 16]

    is stored in memory as: ``[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16]``.

- In a row-major format, the same matrix:

    ::

            [1  2  3  4]
            [5  6  7  8]
            [9  10 11 12]
            [13 14 15 16]

    is stored in memory as: ``[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16]``.

If ``TP_DIM_A_LEADING`` is set to 0, indicating row-major format, the graph will configure DMA buffer descriptors to transpose the data for the kernel. However, this feature is only compatible with ``TT_DATA_A`` being ``int32``, ``cint16``, or ``float`` for a single frame per buffer (``TP_NUM_FRAMES = 1``). For other cases, including ``int16``, ``cint32``, or ``cfloat``, the matrix data must be transposed to a column-major format, and ``TP_DIM_A_LEADING`` must be set to 1.

Additionally, when ``TP_USE_MATRIX_RELOAD = 1``, the matrix data must always be in column-major format, and ``TP_DIM_A_LEADING`` must be set to 1.

Maximum matrix dimensions per kernel
------------------------------------

The maximum memory accessible by a kernel is 32 kB for AIE devices and 64kB for AIE-ML and AIE-MLv2 devices. The maximum matrix dimensions per kernel are limited by the memory requirements and how much memory is available.

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

The following code example demonstrates the usage of the `matrix_vector_mul_graph` class within a user-defined super-graph. It illustrates the multiplication of a 32x16 `cint16` matrix with a `cint16` vector of length 16, showcasing the setup and connection of input/output ports and template parameters.

.. literalinclude:: ../../../../L2/examples/docs_examples/test_matvec.hpp
    :language: cpp
    :lines: 17-


.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim: