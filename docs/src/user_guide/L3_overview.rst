.. 
   Copyright 2019 Xilinx, Inc.
  
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
  
       http://www.apache.org/licenses/LICENSE-2.0
  
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

.. _user_guide_overview_l3:

=====================
L3 API Overview
=====================
XFBLAS level 3 provides software API functions to offload BLAS operations to pre-built FPGA images. 

1. Introduction
================
The XFBLAS level 3 library is an implementation of BLAS on top of the XILINX runtime (XRT). It allows software developers to use XFBLAS library without writing any runtime functions and hardware configurations. 

1.1 Data layout
---------------
XFBLAS library uses row-major storage. The array index of a matrix element could be calculated by the following macro.
  
.. code-block::

  # define IDX2R(i,j,ld) ( ( (i)*(ld) ) + (j) )
  
1.2 Memory Allocation
----------------------
XFBLAS level 3 library supports three different versions of APIs to support memory allocation in device. Users could choose from different versions to better support their application based on the cases.

+--------------------------------+-------------------------------------+------------------------------+
| Already have host memory input | Host memory matrix sizes are padded | Version to choose            |
+================================+=====================================+==============================+
| Yes                            | Yes                                 | Restricted memory version    |
+--------------------------------+-------------------------------------+------------------------------+
| Yes                            | No                                  | Default memory version       |
+--------------------------------+-------------------------------------+------------------------------+
| No                             | Does not apply                      | Pre-allocated memory version |
+--------------------------------+-------------------------------------+------------------------------+

1.3 Supported Datatypes
------------------------
- short
- float

2. Using the XFBLAS API
========================

2.1 General description
------------------------
This section describes how to use the XFBLAS library API level.

2.1.1 Error status
~~~~~~~~~~~~~~~~~~~
XFBLAS API function calls return the error status of datatype `xfblasStatus_t <2.2.2 xfblasStatus_t_>`_.

2.1.2 XFBLAS context
~~~~~~~~~~~~~~~~~~~~
.. NOTE:: TODO

2.2 Datatypes Reference
-----------------------

2.2.1 xfblasHandle_t
~~~~~~~~~~~~~~~~~~~~~~
.. NOTE:: TODO

2.2.2 xfblasStatus_t
~~~~~~~~~~~~~~~~~~~~~~
The type is used for function status returns. All XFBLAS level 3 library functions return status which has the following values.

+-------------------------------+-------------------------------------------------------------------------------------------------------------------+--------+
| Item                          | Meaning                                                                                                           | Value  |
+===============================+===================================================================================================================+========+
| XFBLAS_STATUS_SUCCESS         | The function is completed successfully                                                                            | 0      |
+-------------------------------+-------------------------------------------------------------------------------------------------------------------+--------+
| XFBLAS_STATUS_NOT_INITIALIZED | The XFBLAS library was not initialized. This is usually caused by not calling function xfblasCreate previously.   | 1      |
+-------------------------------+-------------------------------------------------------------------------------------------------------------------+--------+
| XFBLAS_STATUS_INVALID_VALUE   | An unsupported value or parameter was passed to the function for example an negative matrix size.                 | 2      |
+-------------------------------+-------------------------------------------------------------------------------------------------------------------+--------+
| XFBLAS_STATUS_ALLOC_FAILED    | Memory allocation failed inside the XFBLAS library.                                                               | 3      |
+-------------------------------+-------------------------------------------------------------------------------------------------------------------+--------+
| XFBLAS_STATUS_NOT_SUPPORTED   | The functionality requested is not supported yet.                                                                 | 4      |
+-------------------------------+-------------------------------------------------------------------------------------------------------------------+--------+
| XFBLAS_STATUS_NOT_PADDED      | For restricted mode, matrix sizes are not padded correctly.                                                       | 5      |
+-------------------------------+-------------------------------------------------------------------------------------------------------------------+--------+

2.2.3 xfblasEngine_t
~~~~~~~~~~~~~~~~~~~~~
The xfblasEngine_t type indicates which engine needs to be performed when initializes the XFBLAS library. xfblasEngine_t type should be matched with the FPGA bitstream.

+--------------------+-----------------------------+
| Value              | Meaning                     |
+====================+=============================+
| XFBLAS_ENGINE_GEMM | The GEMM engine is selected |
+--------------------+-----------------------------+
| XFBLAS_ENGINE_GEMV | The GEMV engine is selected |
+--------------------+-----------------------------+


2.2.4 xfblasOperation_t
~~~~~~~~~~~~~~~~~~~~~~~~
The xfblasOperation_t type indicates which operation needs to be performed with the matrix.

+-------------+-----------------------------------------------+
| Value       | Meaning                                       |
+=============+===============================================+
| XFBLAS_OP_N | The non-transpose operation is selected       |
+-------------+-----------------------------------------------+
| XFBLAS_OP_T | The transpose operation is selected           |
+-------------+-----------------------------------------------+
| XFBLAS_OP_C | The conjugate transpose operation is selected |
+-------------+-----------------------------------------------+

2.3 XFBLAS Helper Function Reference
-------------------------------------

2.3.1 xfblasCreate
~~~~~~~~~~~~~~~~~~~

.. ref-code-block:: cpp
    :class: title-code-block

    xfblasStatus_t xfblasCreate(const char* xclbin, string configFile, const char* logFile, xfblasEngine_t engineName, unsigned int PE = 0)

This function initializes the XFBLAS library and creates a handle for the specific engine. It must be called prior to any other XFBLAS library calls.

.. rubric:: Parameters:

.. list-table::
    :widths: 20 80

    *
        - xclbin

        - file path to FPGA bitstream

    *
        - configFile

        - file path to config_info.dat file

    *
        - logFile

        - file path to log file

    *
        - engineName

        - XFBLAS engine to run

    *
        - PE

        - index of kernel that is being used, default is 0

.. rubric:: Return:

.. list-table::
    :widths: 20 80
    
    *
        - xfblasStatus_t

        - 0 if the initialization succeeded

    *
        - xfblasStatus_t

        - 1 if the opencl runtime initialization failed

    *
        - xfblasStatus_t

        - 2 if the xclbin doesn't contain the engine

    *
        - xfblasStatus_t

        - 4 if the engine is not supported for now

2.3.2 xfblasMalloc
~~~~~~~~~~~~~~~~~~~
        
.. ref-code-block:: cpp
    :class: title-code-block

    xfblasStatus_t xfblasMalloc(short** devPtr, int rows, int lda, int elemSize)
    xfblasStatus_t xfblasMalloc(float ** devPtr, int rows, int lda, int elemSize)

This function allocates memory on the FPGA device.

.. rubric:: Parameters:

.. list-table::
    :widths: 20 80

    *
        - devPtr

        - pointer to mapped memory

    *
        - rows

        - number of rows in the matrix

    *
        - lda

        - leading dimension of the matrix that indicates the total number of cols in the matrix

    *
        - elemSize

        - number of bytes required to store each element in the matrix

.. rubric:: Return:

.. list-table::
    :widths: 20 80
    
    *
        - xfblasStatus_t

        - 0 if the allocation completed successfully

    *
        - xfblasStatus_t

        - 1 if the library was not initialized

    *
        - xfblasStatus_t

        - 2 if parameters rows, cols, elemSize, lda <= 0 or cols > lda or data types are not matched

    *
        - xfblasStatus_t

        - 3 if there is memory already allocated to the same matrix

    *
        - xfblasStatus_t

        - 4 if the engine is not supported for now

2.3.3 xfblasMallocRestricted
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. ref-code-block:: cpp
    :class: title-code-block

    xfblasStatus_t xfblasMallocRestricted(int rows, int cols, int elemSize, void* A, int lda)

This function allocates memory for host row-major format matrix on the FPGA device.

.. rubric:: Parameters:

.. list-table::
    :widths: 20 80

    *
        - rows

        - number of rows in the matrix

    *
        - cols

        - number of cols in the matrix that is being used

    *
        - elemSize

        - number of bytes required to store each element in the matrix

    *
        - A

        - pointer to the matrix array in the host memory

    *
        - lda

        - leading dimension of the matrix that indicates the total number of cols in the matrix

.. rubric:: Return:

.. list-table::
    :widths: 20 80
    
    *
        - xfblasStatus_t

        - 0 if the allocation completed successfully

    *
        - xfblasStatus_t

        - 1 if the library was not initialized

    *
        - xfblasStatus_t

        - 2 if parameters rows, cols, elemSize, lda <= 0 or cols > lda or data types are not matched

    *
        - xfblasStatus_t

        - 3 if there is memory already allocated to the same matrix

    *
        - xfblasStatus_t

        - 4 if the engine is not supported for now

    *
        - xfblasStatus_t

        - 5 if rows, cols or lda is not padded correctly

2.3.4 xfblasSetMatrix
~~~~~~~~~~~~~~~~~~~~~~

.. ref-code-block:: cpp
    :class: title-code-block

    xfblasStatus_t xfblasSetMatrix(int rows, int cols, int elemSize, short* A, int lda, short* d_A)
    xfblasStatus_t xfblasSetMatrix(int rows, int cols, int elemSize, float* A, int lda, float* d_A)

This function copies a matrix in host memory to FPGA device memory. `xfblasMalloc() <2.3.2 xfblasMalloc_>`_ need to be called prior to this function.

.. rubric:: Parameters:

.. list-table::
    :widths: 20 80

    *
        - rows

        - number of rows in the matrix

    *
        - cols

        - number of cols in the matrix that is being used

    *
        - elemSize

        - number of bytes required to store each element in the matrix

    *
        - A

        - pointer to the matrix array in the host memory

    *
        - lda

        - leading dimension of the matrix that indicates the total number of cols in the matrix

    *
        - d_A

        - pointer to mapped memory

.. rubric:: Return:

.. list-table::
    :widths: 20 80
    
    *
        - xfblasStatus_t

        - 0 if the operation completed successfully

    *
        - xfblasStatus_t

        - 1 if the library was not initialized

    *
        - xfblasStatus_t

        - 2 if parameters rows, cols, elemSize, lda <= 0 or cols > lda or data types are not matched

    *
        - xfblasStatus_t

        - 3 if there is no FPGA device memory allocated for the matrix

    *
        - xfblasStatus_t

        - 4 if the engine is not supported for now

2.3.5 xfblasSetMatrixRestricted
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. ref-code-block:: cpp
    :class: title-code-block

    xfblasStatus_t xfblasSetMatrixRestricted(void* A)

This function copies a matrix in host memory to FPGA device memory. `xfblasMallocRestricted() <2.3.3 xfblasMallocRestricted_>`_ need to be called prior to this function.

.. rubric:: Parameters:

.. list-table::
    :widths: 20 80

    *
        - A

        - pointer to the matrix array in the host memory

.. rubric:: Return:

.. list-table::
    :widths: 20 80
    
    *
        - xfblasStatus_t

        - 0 if the operation completed successfully

    *
        - xfblasStatus_t

        - 1 if the library was not initialized

    *
        - xfblasStatus_t

        - 3 if there is no FPGA device memory allocated for the matrix

2.3.6 xfblasGetMatrix
~~~~~~~~~~~~~~~~~~~~~~~

.. ref-code-block:: cpp
    :class: title-code-block

    xfblasStatus_t xfblasGetMatrix(int rows, int cols, int elemSize, short* d_a, short* a, int lda)
    xfblasStatus_t xfblasGetMatrix(int rows, int cols, int elemSize, float* d_a, float* a, int lda) 

This function copies a matrix in FPGA device memory to host memory.

.. rubric:: Parameters:

.. list-table::
    :widths: 20 80

    *
        - A

        - pointer to matrix A in the host memory

.. rubric:: Return:

.. list-table::
    :widths: 20 80
    
    *
        - xfblasStatus_t

        - 0 if the operation completed successfully

    *
        - xfblasStatus_t

        - 1 if the library was not initialized

    *
        - xfblasStatus_t

        - 3 if there is no FPGA device memory allocated for the matrix

2.3.7 xfblasGetMatrixRestricted
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. ref-code-block:: cpp
    :class: title-code-block

    xfblasStatus_t xfblasGetMatrixRestricted(void* A)

This function copies a matrix in FPGA device memory to host memory.

.. rubric:: Parameters:

.. list-table::
    :widths: 20 80

    *
        - A

        - pointer to matrix A in the host memory

.. rubric:: Return:

.. list-table::
    :widths: 20 80
    
    *
        - xfblasStatus_t

        - 0 if the operation completed successfully

    *
        - xfblasStatus_t

        - 1 if the library was not initialized

    *
        - xfblasStatus_t

        - 3 if there is no FPGA device memory allocated for the matrix

2.3.8 xfblasFree
~~~~~~~~~~~~~~~~~

.. ref-code-block:: cpp
    :class: title-code-block

    xfblasStatus_t xfblasFree(void* A)

This function frees memory in FPGA device.

.. rubric:: Parameters:

.. list-table::
    :widths: 20 80

    *
        - A

        - pointer to matrix A in the host memory

.. rubric:: Return:

.. list-table::
    :widths: 20 80
    
    *
        - xfblasStatus_t

        - 0 if the operation completed successfully

    *
        - xfblasStatus_t

        - 1 if the library was not initialized

    *
        - xfblasStatus_t

        - 3 if there is no FPGA device memory allocated for the matrix

2.3.9 xfblasDestory
~~~~~~~~~~~~~~~~~~~~

.. ref-code-block:: cpp
    :class: title-code-block

    xfblasStatus_t xfblasDestory()

This function releases handle used by the XFBLAS library.

.. rubric:: Return:

.. list-table::
    :widths: 20 80

    *
        - xfblasStatus_t

        - 0 if the shut down succeeded

    *
        - xfblasStatus_t

        - 1 if the library was not initialized

2.4 XFBLAS Function Reference
------------------------------

2.4.1 xfblasGemm
~~~~~~~~~~~~~~~~~~

.. ref-code-block:: cpp
    :class: title-code-block

    xfblasStatus_t xfblasGemm(xfblasOperation_t transa, xfblasOperation_t transb, int m, int n, int k, int alpha, void* A, int lda, void* B, int ldb, int beta, void* C, int ldc)

This function performs the matrix-matrix multiplication C = alpha*op(A)op(B) + beta*C. See :doc:`gemm example<L3_example_gemm>` for detail usage.

.. rubric:: Parameters:

.. list-table::
    :widths: 20 80

    *
        - transa

        - operation op(A) that is non- or (conj.) transpose

    *
        - transb

        - operation op(B) that is non- or (conj.) transpose

    *
        - m

        - number of rows in matrix A, matrix C

    *
        - n

        - number of cols in matrix B, matrix C

    *
        - k

        - number of cols in matrix A, number of rows in matrix B

    *
        - alpha

        - scalar used for multiplication

    *
        - A

        - pointer to matrix A in the host memory

    *
        - lda

        - leading dimension of matirx A

    *
        - B

        - pointer to matrix B in the host memory

    *
        - ldb

        - leading dimension of matrix B

    *
        - beta

        - scalar used for multiplication

    *
        - C

        - pointer to matrix C in the host memory

    *
        - ldc

        - leading dimension of matrix C

.. rubric:: Return:

.. list-table::
    :widths: 20 80
    
    *
        - xfblasStatus_t

        - 0 if the operation completed successfully

    *
        - xfblasStatus_t

        - 1 if the library was not initialized

    *
        - xfblasStatus_t

        - 3 if not all the matrices have FPGA devie memory allocated

    *
        - xfblasStatus_t

        - 4 if the engine is not supported for now

        
3. Obtain FPGA bitstream 
=========================
.. NOTE:: TODO
