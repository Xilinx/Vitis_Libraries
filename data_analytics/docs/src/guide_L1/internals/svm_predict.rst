.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _guide-svm_predict:

********************************************************
Internals of svm_predict
********************************************************

.. toctree::
   :hidden:
   :maxdepth: 1

This document describes the structure and execution of svm prediction, implemented as a ``svmPredict`` function.

Svm_predict is a function to predict a sample's classification by trained weight vector and sample feature vector.  This function provide a stream in, stream out module to easily get the prediction class of a sample.

The structure of ``svmPredict`` is described as follows. The primitive have two function which are ``getWeight`` and ``getPredictions``. The input of ``getWeight`` is stream, and this part stores the weight vector in RAM. ``getPredictions`` has a dataflow region including two functions. One is dot_multiply, and the other is tag type transform.

.. image:: /images/svm_prediction.png
   :alt: svm_prediction Top Structure
   :align: center

The hardware resource utilization of svm_prediction is shown in the following table (work as 300 MHz). 

+--------+----------+--------+------+-----+
|  LUT   |    FF    |  BRAM  | URAM | DSP |
+--------+----------+--------+------+-----+
| 27114  |  37335   |   26   |  0   | 133 |
+--------+----------+--------+------+-----+
