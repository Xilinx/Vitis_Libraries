.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _guide-kMeansPredict:

********************************
K-Means (Predict)
********************************

.. toctree::
   :hidden:
   :maxdepth: 2

This document describes the structure and execution of kMeansPredict.

.. image:: /images/kMeanPredict.png
   :alt: k-means prediction Structure
   :width: 80%
   :align: center

kMeansPredict provides prediction the cluster index for each sample, in which the centers are stored in an array whose first dimension should partitioned in its definition. To achieve to accelertion prediction, DV elements in a sample are input at the same time and used for computing distance with KU centers. The static configures are set by template parameters and dynamic by arguments of the API in which dynamic ones should not be greater than static ones. 

There are applicable conditions:

1. All centers are stored in local buffer, so Dim*Kcluster should less than a fixed value. For example, Dim*Kcluster<=1024*1024 for centers with float stored in URAM and 1024*512 for double on an AMD Alveo™ U250.

2. KU and DV should be configured properly due to limitations to the local memory. For example, KU*DV=128 when centers are stored in URAM on an Alveo U250.

3. The dynamic confugures should close to static ones to void unuseful computing inside.