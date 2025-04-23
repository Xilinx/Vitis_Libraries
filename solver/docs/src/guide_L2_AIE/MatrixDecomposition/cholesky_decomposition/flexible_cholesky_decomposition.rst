.. 
   
.. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Flexible Cholesky Decomposition
   :description: This function computes the Cholesky decomposition of matrix using a flexible way.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

*********************************
Flexible Cholesky Decomposition 
*********************************

Introduction
==============

This function computes the Cholesky decomposition of matrix :math:`A` , where input matrix :math:`A` is a Hermitian positive-definite matrix of size :math:`n \times n`, output matrix :math:`L` is a lower triangular matrix with real and positive diagonal entries, and :math:`{L}^*` denotes the conjugate transpose of matrix of :math:`L`.
Every Hermitian positive-definite matrix has a unique Cholesky decomposition.

.. math::
    A = L {L}^*

In this design, customer could instantiate some parameters to trade-off AIE tile resouces for throughput. 

Entry Point
==============

The graph entry point is the following:

.. code::

    xf::solver::CholeskyFlexibleGraph

Template Parameters
---------------------
* `Dim`: the dimensition of input matrix.
* `CoreNum`: the number of AIE cores used.
* `BlkNum`: the number of matrix columns are calculated by each core.

To access more details, see :ref:`AIE APIs Overview`.

Ports
-------
To access more details, see :ref:`AIE APIs Overview`.


AIE Kernel 
==============

Design Notes
---------------

* Target: :math:`A=LL^*`, :math:`A[N*N]` is input matrix, :math:`L[N*N]` is the output matrix via cholesky decomposition. 
* DataType: `cfloat` is supported.
* DataSize: input matrix dimension :math:`N` is times of `BlkNum`, `BlkNum` is the number of columns calculated by each core. :math:`N` shoulb be no more than 256.
* Description: 
    * For each AIE core,  `BlkNum` columns of :math:`matL` are calculated,  and the related elements of :math:`matA` are updated. The number of `BlkNum` could be instantiated by users.
    * To calculate all columns of :math:`matL`, `CoreNum` AIE cores are used, :math:`CoreNum = N/BlkNum`. 
    * The first core's output is fed to the second core's input, and continued till the last column is computed done.

Kernel Interfaces
--------------------

.. code::

    void CholeskyComplexFlexible::run(
                         adf::input_buffer<cfloat, adf::extents<adf::inherited_extent> >& __restrict in,
                         adf::output_buffer<cfloat, adf::extents<adf::inherited_extent> >& __restrict out);

* Input:

  *  ``input_buffer<cfloat>* in``    input buffer port, contains the input matrix datas column by column. 

* Output:

  *  ``output_buffer<cfloat>* out``  output buffer port, contains the output matrix datas column by column.

.. note::
   * The function assumes that the input matrix is a Hermitian positive definite matrix.



Performance
==============

Test
--------------------
* DataSize: matrix size is 128x128
* DataType: cflaot
* BlkNum: 4
* CoreNum: 32
* AIE simulation time: 311us


.. toctree::
   :maxdepth: 1

