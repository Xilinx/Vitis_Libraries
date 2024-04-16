.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _L2_spmv_double_api:

************************************
Double Precision SPMV Kernel APIs
************************************


.. toctree::
      :maxdepth: 1

.. NOTE::
   The double precision SPMV implementation in the current release uses 16 HBM channels to store NNZ values and indices, 1 HBM channel to store input dense vector X, 2 HBM channels to store partition parameters, and 1 HBM channel to store result Y vector.

.. include:: ../../rst/global.rst
      :start-after: _cid-assembleykernel:
