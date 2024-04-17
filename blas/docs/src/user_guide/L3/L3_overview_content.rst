.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: BLAS, Library, Vitis BLAS Library, Vitis BLAS, level 3
   :description: Vitis BLAS library level 3 provides software API functions to offload BLAS operations to pre-built FPGA images.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


.. _user_guide_overview_content_l3:

The AMD Vitis™ BLAS level 3 provides software API functions to offload BLAS operations to pre-built FPGA images. 

1. Introduction
================

The Vitis BLAS level 3 library is an implementation of BLAS on top of XRT. It allows software developers to use the Vitis BLAS library without writing any runtime functions and hardware configurations. 

1.1 Data Layout
---------------

The Vitis BLAS library uses row-major storage. The array index of a matrix element can be calculated by the following macro:
  
.. code-block::

  # define IDX2R(i,j,ld) ( ( (i)*(ld) ) + (j) )
  
1.2 Memory Allocation
----------------------

The Vitis BLAS level 3 library supports three different versions of APIs to support memory allocation in the device. You can choose from different versions to better support your application based on the cases. Examples for using three different versions can be found in :doc:`BLAS Level 3 example<L3_example>`.

+--------------------------------+-------------------------------------+------------------------------+
| Already Have Host Memory Input | Host Memory Matrix Sizes are Padded | Version to Choose            |
+================================+=====================================+==============================+
| Yes                            | Yes                                 | Restricted memory version    |
+--------------------------------+-------------------------------------+------------------------------+
| Yes                            | No                                  | Default memory version       |
+--------------------------------+-------------------------------------+------------------------------+
| No                             | Does not apply                      | Pre-allocated memory version |
+--------------------------------+-------------------------------------+------------------------------+ 

Restricted Memory Version
^^^^^^^^^^^^^^^^^^^^^^^^^^
  To use restricted memory version, your input matrix sizes must be a multiple of certain configuration values that are used to build the FPGA bitstreams. Also, host memory is encouraged to be 4k aligned when using the restricted memory version. Compared to the default memory version, even though there are requirements on the matrix sizes, restricted memory version can save extra memory copy in the host side. 

Default Memory Version
^^^^^^^^^^^^^^^^^^^^^^^

  This version has no limitations on user host memory, and it is easy to use. API functions will do the padding internally so this will lead to an extra memory copy in the host side. The resulting output matrix will also be the same sizes.
  
Pre-allocated Memory Version
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  To use this version, you need to call API functions to allocate the device memory first, then fill in the host memory that is mapped to the device memory with values. There is no extra memory copy, and the programming is easier compared to the other two versions. However, when filling in the matrices, you need to use the padded sizes; the result output matrix's sizes are padded instead of the original ones. For more usage information, see the examples. 
  
  
1.3 Supported Datatypes
------------------------
- float

2. Using the Vitis BLAS API
=============================

2.1 General Description
------------------------

This section describes how to use the Vitis BLAS library API level.

2.1.1 Error Status
^^^^^^^^^^^^^^^^^^^

The Vitis BLAS API function calls return the error status of the `xfblasStatus_t <2.2.1 xfblasStatus_t_>`_ datatype.

2.1.2 Vitis BLAS Initialization
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To initialize the library, the `xfblasCreate()` function must be called. This function will open the device, download the FPGA image to the device, and create context on the selected compute unit. For multi-kernel xclbins, the contexts will be opened on the corresponding compute units. For detailed usage, refer to the L3 examples.

2.2 Datatypes Reference
-----------------------

2.2.1 xfblasStatus_t
^^^^^^^^^^^^^^^^^^^^^^

The type is used for function status returns. All Vitis BLAS level 3 library functions return status which has the following values.

+-------------------------------+-----------------------------------------------------------------------------------------------------------------------+--------+
| Item                          | Meaning                                                                                                               | Value  |
+===============================+=======================================================================================================================+========+
| XFBLAS_STATUS_SUCCESS         | The function is completed successfully.                                                                                | 0     |
+-------------------------------+-----------------------------------------------------------------------------------------------------------------------+--------+
| XFBLAS_STATUS_NOT_INITIALIZED | The Vitis BLAS library was not initialized. This is usually caused by not calling function xfblasCreate previously.   | 1      |
+-------------------------------+-----------------------------------------------------------------------------------------------------------------------+--------+
| XFBLAS_STATUS_INVALID_VALUE   | An unsupported value or parameter was passed to the function. For example, an negative matrix size.                   | 2      |
+-------------------------------+-----------------------------------------------------------------------------------------------------------------------+--------+
| XFBLAS_STATUS_ALLOC_FAILED    | Memory allocation failed inside the Vitis BLAS library.                                                               | 3      |
+-------------------------------+-----------------------------------------------------------------------------------------------------------------------+--------+
| XFBLAS_STATUS_NOT_SUPPORTED   | The functionality requested is not supported yet.                                                                     | 4      |
+-------------------------------+-----------------------------------------------------------------------------------------------------------------------+--------+
| XFBLAS_STATUS_NOT_PADDED      | For restricted mode, matrix sizes are not padded correctly.                                                           | 5      |
+-------------------------------+-----------------------------------------------------------------------------------------------------------------------+--------+

2.2.2 xfblasEngine_t
^^^^^^^^^^^^^^^^^^^^^

The xfblasEngine_t type indicates which engine needs to be performed when initializing the Vitis BLAS library. The xfblasEngine_t type should be matched with the FPGA bitstream.

+--------------------+-----------------------------+
| Value              | Meaning                     |
+====================+=============================+
| XFBLAS_ENGINE_GEMM | The GEMM engine is selected.|
+--------------------+-----------------------------+


2.2.3 xfblasOperation_t
^^^^^^^^^^^^^^^^^^^^^^^^

The xfblasOperation_t type indicates which operation needs to be performed with the matrix.

+-------------+-----------------------------------------------+
| Value       | Meaning                                       |
+=============+===============================================+
| XFBLAS_OP_N | The non-transpose operation is selected.      |
+-------------+-----------------------------------------------+
| XFBLAS_OP_T | The transpose operation is selected.          |
+-------------+-----------------------------------------------+
| XFBLAS_OP_C | The conjugate transpose operation is selected.|
+-------------+-----------------------------------------------+

2.3 Vitis BLAS Helper Function Reference
----------------------------------------------

2.3.1 xfblasCreate
^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
    :class: title-code-block

    xfblasStatus_t xfblasCreate(const char* xclbin, string configFile, const char* logFile, xfblasEngine_t engineName, unsigned int kernelNumber = 1, unsigned int deviceIndex = 0)

This function initializes the Vitis BLAS library and creates a handle for the specific engine. It must be called prior to any other Vitis BLAS library calls.

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
        - Vitis BLAS engine to run
    *
        - kernelNumber
        - number of kernels that is being used, default is 1
    *
        - deviceIndex
        - index of device that is being used, default is 0

.. rubric:: Return:

.. list-table::
    :widths: 20 80
    
    *
        - xfblasStatus_t
        - 0 if the initialization succeeded.
    *
        - xfblasStatus_t
        - 1 if the opencl runtime initialization failed.
    *
        - xfblasStatus_t
        - 2 if the xclbin does not contain the engine.
    *
        - xfblasStatus_t
        - 4 if the engine is not supported for now.

2.3.2 xfblasFree
^^^^^^^^^^^^^^^^^

.. code-block:: cpp
    :class: title-code-block

    xfblasStatus_t xfblasFree(void* A, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0)

This function frees memory in the FPGA device.

.. rubric:: Parameters:

.. list-table::
    :widths: 20 80

    *
        - A
        - Pointer to matrix A in the host memory.
    *
        - kernelIndex
        - Index of kernel that is being used; default is 0.
    *
        - deviceIndex
        - Index of device that is being used; default is 0.


.. rubric:: Return:

.. list-table::
    :widths: 20 80
    
    *
        - xfblasStatus_t
        - 0 if the operation completed successfully.
    *
        - xfblasStatus_t
        - 1 if the library was not initialized.
    *
        - xfblasStatus_t
        - 3 if there is no FPGA device memory allocated for the matrix.
        
2.3.3 xfblasDestroy
^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
    :class: title-code-block

    xfblasStatus_t xfblasDestroy(unsigned int kernelNumber = 1, unsigned int deviceIndex = 0)

This function releases the handle used by the Vitis BLAS library.

.. rubric:: Parameters:

.. list-table::
    :widths: 20 80

    *
        - kernelNumber
        - number of kernels that is being used, default is 1
    *
        - deviceIndex
        - index of device that is being used, default is 0
        
.. rubric:: Return:

.. list-table::
    :widths: 20 80

    *
        - xfblasStatus_t
        - 0 if the shut down succeeded.
    *
        - xfblasStatus_t
        - 1 if the library was not initialized.
        
2.3.4 xfblasMalloc
^^^^^^^^^^^^^^^^^^^
        
.. code-block:: cpp
    :class: title-code-block

    xfblasStatus_t xfblasMalloc(short** devPtr, int rows, int lda, int elemSize, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0)
    xfblasStatus_t xfblasMalloc(float** devPtr, int rows, int lda, int elemSize, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0)

This function allocates memory on the FPGA device.

.. rubric:: Parameters:

.. list-table::
    :widths: 20 80

    *
        - devPtr
        - Pointer to the mapped memory.
    *
        - rows
        - Number of rows in the matrix.
    *
        - lda
        - Leading dimension of the matrix that indicates the total number of cols in the matrix.
    *
        - elemSize
        - Number of bytes required to store each element in the matrix.
    *
        - kernelIndex
        - Index of the kernel that is being used; default is 0.
    *
        - deviceIndex
        - Index of the device that is being used; default is 0.
        
.. rubric:: Return:

.. list-table::
    :widths: 20 80
    
    *
        - xfblasStatus_t
        - 0 if the allocation completed successfully.
    *
        - xfblasStatus_t
        - 1 if the library was not initialized.
    *
        - xfblasStatus_t
        - 2 if the parameters rows, cols, elemSize, lda <= 0 or cols > lda or data types are not matched.
    *
        - xfblasStatus_t
        - 3 if there is memory already allocated to the same matrix.
    *
        - xfblasStatus_t
        - 4 if the engine is not supported for now.

2.3.5 xfblasSetVector
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
    :class: title-code-block

    xfblasStatus_t xfblasSetVector(int n, int elemSize, short* x, int incx, short* d_x, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0)
    xfblasStatus_t xfblasSetVector(int n, int elemSize, float* x, int incx, float* d_x, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0)

This function copies a vector in the host memory to ythe FPGA device memory. `xfblasMalloc() <2.3.4 xfblasMalloc_>`_ need to be called prior to this function.

.. rubric:: Parameters:

.. list-table::
    :widths: 20 80

    *
        - n
        - Number of elements in the vector.
    *
        - elemSize
        - Number of bytes required to store each element in the vector.
    *
        - x
        - Pointer to the vector in the host memory.
    *
        - incx
        - The storage spacing between consecutive elements of vector x.
    *
        - d_x
        - Pointer to the mapped memory.
    *
        - kernelIndex
        - Index of the kernel that is being used; default is 0.
    *
        - deviceIndex
        - Index of the device that is being used; default is 0.
        
.. rubric:: Return:

.. list-table::
    :widths: 20 80

    *
        - xfblasStatus_t
        - 0 if the operation completed successfully.
    *
        - xfblasStatus_t
        - 1 if the library was not initialized.
    *
        - xfblasStatus_t
        - 2 if parameters rows, cols, elemSize, lda <= 0 or cols > lda or data types are not matched.
    *
        - xfblasStatus_t
        - 3 if there is no FPGA device memory allocated for the vector.
    *
        - xfblasStatus_t
        - 4 if the engine is not supported for now.

2.3.6 xfblasGetVector
^^^^^^^^^^^^^^^^^^^^^^
        
.. code-block:: cpp
    :class: title-code-block

    xfblasStatus_t xfblasGetVector(int n, int elemSize, short* d_x, short* x, int incx, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0)
    xfblasStatus_t xfblasGetVector(int n, int elemSize, float* d_x, float* x, int incx, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0)

This function copies a vector in the FPGA device memory to the host memory.

.. rubric:: Parameters:

.. list-table::
    :widths: 20 80

    *
        - n
        - Number of elements in the vector.
    *
        - elemSize
        - Number of bytes required to store each element in the vector.
    *
        - d_x
        - Pointer to the mapped memory.
    *
        - x
        - Pointer to the vector in the host memory.
    *
        - incx
        - The storage spacing between the consecutive elements of vector x.
    *
        - kernelIndex
        - Index of the kernel that is being used; default is 0.
    *
        - deviceIndex
        - Index of the device that is being used; default is 0.
        
.. rubric:: Return:

.. list-table::
    :widths: 20 80

    *
        - xfblasStatus_t
        - 0 if the operation completed successfully.
    *
        - xfblasStatus_t
        - 1 if the library was not initialized.
    *
        - xfblasStatus_t
        - 3 if there is no FPGA device memory allocated for the vector.

2.3.7 xfblasSetMatrix
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
    :class: title-code-block

    xfblasStatus_t xfblasSetMatrix(int rows, int cols, int elemSize, short* A, int lda, short* d_A, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0)
    xfblasStatus_t xfblasSetMatrix(int rows, int cols, int elemSize, float* A, int lda, float* d_A, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0)

This function copies a matrix in the host memory to the FPGA device memory. `xfblasMalloc() <2.3.4 xfblasMalloc_>`_ need to be called prior to this function.

.. rubric:: Parameters:

.. list-table::
    :widths: 20 80

    *
        - rows
        - Number of rows in the matrix.
    *
        - cols
        - Number of cols in the matrix that is being used.
    *
        - elemSize
        - Number of bytes required to store each element in the matrix.
    *
        - A
        - Pointer to the matrix array in the host memory.
    *
        - lda
        - Leading dimension of the matrix that indicates the total number of cols in the matrix.
    *
        - d_A
        - Pointer to mapped memory.
    *
        - kernelIndex
        - Index of kernel that is being used; default is 0.
    *
        - deviceIndex
        - Index of device that is being used; default is 0.
        
.. rubric:: Return:

.. list-table::
    :widths: 20 80
    
    *
        - xfblasStatus_t
        - 0 if the operation completed successfully.
    *
        - xfblasStatus_t
        - 1 if the library was not initialized.
    *
        - xfblasStatus_t
        - 2 if parameters rows, cols, elemSize, lda <= 0 or cols > lda or data types are not matched.
    *
        - xfblasStatus_t
        - 3 if there is no FPGA device memory allocated for the matrix.
    *
        - xfblasStatus_t
        - 4 if the engine is not supported for now.

2.3.8 xfblasGetMatrix
^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
    :class: title-code-block

    xfblasStatus_t xfblasGetMatrix(int rows, int cols, int elemSize, short* d_A, short* A, int lda, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0)
    xfblasStatus_t xfblasGetMatrix(int rows, int cols, int elemSize, float* d_A, float* A, int lda, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0) 

This function copies a matrix in the FPGA device memory to the host memory.

.. rubric:: Parameters:

.. list-table::
    :widths: 20 80

    *
        - rows
        - Number of rows in the matrix.
    *
        - cols
        - Number of cols in the matrix that is being used.

    *
        - elemSize
        - Number of bytes required to store each element in the matrix.
    *
        - d_A
        - Pointer to mapped memory.
    *
        - A
        - Pointer to the matrix array in the host memory.
    *
        - lda
        - Leading dimension of the matrix that indicates the total number of cols in the matrix.
    *
        - kernelIndex
        - Index of the kernel that is being used; default is 0.
    *
        - deviceIndex
        - Index of the device that is being used; default is 0.
        
.. rubric:: Return:

.. list-table::
    :widths: 20 80
    
    *
        - xfblasStatus_t
        - 0 if the operation completed successfully.
    *
        - xfblasStatus_t
        - 1 if the library was not initialized.
    *
        - xfblasStatus_t
        - 3 if there is no FPGA device memory allocated for the matrix.
        
2.3.9 xfblasMallocRestricted
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
    :class: title-code-block

    xfblasStatus_t xfblasMallocRestricted(int rows, int cols, int elemSize, void* A, int lda, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0)

This function allocates memory for the host row-major format matrix on the FPGA device.

.. rubric:: Parameters:

.. list-table::
    :widths: 20 80

    *
        - rows
        - Number of rows in the matrix.
    *
        - cols
        - Number of cols in the matrix that is being used.
    *
        - elemSize
        - Number of bytes required to store each element in the matrix.
    *
        - A
        - Pointer to the matrix array in the host memory.
    *
        - lda
        - Leading dimension of the matrix that indicates the total number of cols in the matrix.
        
    *
        - kernelIndex
        - Index of the kernel that is being used; default is 0.
    *
        - deviceIndex
        - Index of the device that is being used; default is 0.
        
.. rubric:: Return:

.. list-table::
    :widths: 20 80
    
    *
        - xfblasStatus_t
        - 0 if the allocation completed successfully.

    *
        - xfblasStatus_t
        - 1 if the library was not initialized.

    *
        - xfblasStatus_t
        - 2 if parameters rows, cols, elemSize, lda <= 0 or cols > lda or data types are not matched.

    *
        - xfblasStatus_t
        - 3 if there is memory already allocated to the same matrix.

    *
        - xfblasStatus_t
        - 4 if the engine is not supported for now.

    *
        - xfblasStatus_t
        - 5 if rows, cols or lda is not padded correctly.

2.3.10 xfblasSetVectorRestricted
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
    :class: title-code-block

    xfblasStatus_t xfblasSetVectorRestricted(void* x, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0)

This function copies a vector in the host memory to the FPGA device memory. `xfblasMallocRestricted() <2.3.9 xfblasMallocRestricted_>`_ need to be called prior to this function.

.. rubric:: Parameters:

.. list-table::
    :widths: 20 80

    *
        - x
        - Pointer to the vector in the host memory.
    *
        - kernelIndex
        - Index of the kernel that is being used; default is 0.
    *
        - deviceIndex
        - Index of the device that is being used; default is 0.
        
.. rubric:: Return:

.. list-table::
    :widths: 20 80
    
    *
        - xfblasStatus_t
        - 0 if the operation completed successfully.
    *
        - xfblasStatus_t
        - 1 if the library was not initialized.
    *
        - xfblasStatus_t
        - 3 if there is no FPGA device memory allocated for the vector.
  
2.3.11 xfblasGetVectorRestricted
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
    :class: title-code-block

    xfblasStatus_t xfblasGetVectorRestricted(void* x, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0)

This function copies a matrix in the FPGA device memory to the host memory.

.. rubric:: Parameters:

.. list-table::
    :widths: 20 80

    *
        - x
        - Pointer to vector x in the host memory.
    *
        - kernelIndex
        - Index of kernel that is being used; default is 0.
    *
        - deviceIndex
        - Index of device that is being used; default is 0.
        
.. rubric:: Return:

.. list-table::
    :widths: 20 80

    *
        - xfblasStatus_t
        - 0 if the operation completed successfully.
    *
        - xfblasStatus_t
        - 1 if the library was not initialized.
    *
        - xfblasStatus_t
        - 3 if there is no FPGA device memory allocated for the matrix.


2.3.12 xfblasSetMatrixRestricted
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
    :class: title-code-block

    xfblasStatus_t xfblasSetMatrixRestricted(void* A, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0)

This function copies a matrix in the host memory to the FPGA device memory. `xfblasMallocRestricted() <2.3.9 xfblasMallocRestricted_>`_ need to be called prior to this function.

.. rubric:: Parameters:

.. list-table::
    :widths: 20 80

    *
        - A
        - Pointer to the matrix array in the host memory.
    *
        - kernelIndex
        - Index of kernel that is being used; default is 0.
    *
        - deviceIndex
        - Index of device that is being used; default is 0.
        
.. rubric:: Return:

.. list-table::
    :widths: 20 80
    
    *
        - xfblasStatus_t
        - 0 if the operation completed successfully.
    *
        - xfblasStatus_t
        - 1 if the library was not initialized.
    *
        - xfblasStatus_t
        - 3 if there is no FPGA device memory allocated for the matrix.

2.3.13 xfblasGetMatrixRestricted
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
    :class: title-code-block

    xfblasStatus_t xfblasGetMatrixRestricted(void* A, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0)

This function copies a matrix in theFPGA device memory to the host memory.

.. rubric:: Parameters:

.. list-table::
    :widths: 20 80

    *
        - A
        - Pointer to matrix A in the host memory.
    *
        - kernelIndex
        - Index of kernel that is being used; default is 0.
    *
        - deviceIndex
        - Index of device that is being used; default is 0.
        
.. rubric:: Return:

.. list-table::
    :widths: 20 80
    
    *
        - xfblasStatus_t
        - 0 if the operation completed successfully.
    *
        - xfblasStatus_t
        - 1 if the library was not initialized.
    *
        - xfblasStatus_t
        - 3 if there is no FPGA device memory allocated for the matrix.

2.3.14 xfblasMallocManaged
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
    :class: title-code-block

    xfblasStatus_t xfblasMallocManaged(short** devPtr, int* paddedLda, int rows, int lda, int elemSize, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0)
    xfblasStatus_t xfblasMallocManaged(float** devPtr, int* paddedLda, int rows, int lda, int elemSize, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0)

This function allocates memory on the FPGA device, and rewrites the leading dimension size after padding.

.. rubric:: Parameters:

.. list-table::
    :widths: 20 80

    *
        - devPtr
        - Pointer to mapped memory.
    *
        - paddedLda
        - Leading dimension of the matrix after padding.
    *
        - rows
        - Number of rows in the matrix.
    *
        - lda
        - Leading dimension of the matrix that indicates the total number of cols in the matrix.
    *
        - elemSize
        - Number of bytes required to store each element in the matrix.     
    *
        - kernelIndex
        - Index of kernel that is being used; default is 0.
    *
        - deviceIndex
        - Index of device that is being used; default is 0.
        
.. rubric:: Return:

.. list-table::
    :widths: 20 80        

    *
        - xfblasStatus_t
        - 0 if the allocation completed successfully.
    *
        - xfblasStatus_t
        - 1 if the library was not initialized.
    *
        - xfblasStatus_t
        - 2 if parameters rows, cols, elemSize, lda <= 0 or cols > lda or data types are not matched.
    *
        - xfblasStatus_t
        - 3 if there is memory already allocated to the same matrix.
    *
        - xfblasStatus_t
        - 4 if the engine is not supported for now.
        
        
2.3.15 xfblasExecute
^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
    :class: title-code-block

    xfblasStatus_t xfblasExecute (
        unsigned int kernelIndex = 0,
        unsigned int deviceIndex = 0
        )

This function starts the kernel and waits until it finishes.

.. rubric:: Parameters:

.. list-table::
    :widths: 20 80
    
    *
        - kernelIndex
        - Index of the kernel that is being used; default is 0.
    *
        - deviceIndex
        - Index of the device that is being used; default is 0.
    *
        - xfblasStatus_t
        - 0 if the operation completed successfully.
    *
        - xfblasStatus_t
        - 1 if the library was not initialized.
    *
        - xfblasStatus_t
        - 3 if there is no FPGA device memory allocated for instrution.


2.3.16 xfblasExecuteAsync
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
    :class: title-code-block

    void xfblasExecuteAsync (
        unsigned int numKernels = 1,
        unsigned int deviceIndex = 0
        )

This asynchronous function starts all kernels and waits until they finish.


.. rubric:: Parameters:

.. list-table::
    :widths: 20 80
    
    *
        - numKernels
        - Number of kernels that is being used; default is 1.
    *
        - deviceIndex
        - Index of device that is being used; default is 0.

2.3.17 xfblasGetByPointer
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
    :class: title-code-block

    xfblasStatus_t xfblasGetByPointer (
        void* A,
        unsigned int kernelIndex = 0,
        unsigned int deviceIndex = 0
        )

This function copies a matrix in the FPGA device memory to the host memory by a pointer.

.. rubric:: Parameters:

.. list-table::
    :widths: 20 80

    *
        - A
        - Pointer to matrix A in the host memory.
    *
        - kernelIndex
        - Index of kernel that is being used; default is 0.
    *
        - deviceIndex
        - Index of device that is being used; default is 0.
    *
        - xfblasStatus_t
        - 0 if the operation completed successfully.
    *
        - xfblasStatus_t
        - 1 if the library was not initialized.
    *
        - xfblasStatus_t
        - 3 if there is no FPGA device memory allocated for the matrix.

2.4 Vitis BLAS Function Reference
-----------------------------------

2.4.1 xfblasGemm
^^^^^^^^^^^^^^^^^^

.. code-block:: cpp
    :class: title-code-block

    xfblasStatus_t xfblasGemm(xfblasOperation_t transa, xfblasOperation_t transb, int m, int n, int k, int alpha, void* A, int lda, void* B, int ldb, int beta, void* C, int ldc, unsigned int kernelIndex = 0, unsigned int deviceIndex = 0)

This function performs the matrix-matrix multiplication C = alpha*op(A)op(B) + beta*C. For detailed usage, see the L3 examples.

.. rubric:: Parameters:

.. list-table::
    :widths: 20 80

    *
        - transa
        - Operation op(A) that is non- or (conj.) transpose.
    *
        - transb
        - Operation op(B) that is non- or (conj.) transpose.
    *
        - m
        - Number of rows in matrix A, matrix C.
    *
        - n
        - Number of cols in matrix B, matrix C.
    *
        - k
        - Number of cols in matrix A, number of rows in matrix B.
    *
        - alpha
        - Scalar used for multiplication.
    *
        - A
        - Pointer to matrix A in the host memory.
    *
        - lda
        - Leading dimension of matrix A.
    *
        - B
        - Pointer to matrix B in the host memory.
    *
        - ldb
        - Leading dimension of matrix B.
    *
        - beta
        - Scalar used for multiplication.
    *
        - C
        - Pointer to matrix C in the host memory.
    *
        - ldc
        - Leading dimension of matrix C.
    *
        - kernelIndex
        - Index of the kernel that is being used; default is 0.
    *
        - deviceIndex
        - Index of the device that is being used; default is 0.
        
.. rubric:: Return:

.. list-table::
    :widths: 20 80
    
    *
        - xfblasStatus_t
        - 0 if the operation completed successfully.
    *
        - xfblasStatus_t
        - 1 if the library was not initialized.
    *
        - xfblasStatus_t
        - 3 if not all the matrices have FPGA devie memory allocated.
    *
        - xfblasStatus_t
        - 4 if the engine is not supported for now.
 
3. Obtain FPGA bitstream 
=========================

FPGA bitstreams can be built in the examples or tests folder by using the `make build TARGET=hw PLATFORM_REPO_PATHS=LOCAL_PLATFORM_PATH` command. 
