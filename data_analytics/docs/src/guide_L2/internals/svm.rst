.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _guide-svm_train:

********************************************************
Internals of svm_train
********************************************************

.. toctree::
   :hidden:
   :maxdepth: 1

Overview
========

Support Vector Machine (SVM) is a model to predict a sample's classification. This document describes the structure and execution of svm train, implemented as ``SVM`` function.

Basic Algorithm
================

Svm_train is a function to trains a batch of samples using the support vector machine model and outputs a weight vecotor for classification. This function used the Stochastic Gradient Descent (SGD) method to get convergence.


Implementation
==============

The structure of ``SVM`` is described as below. It has a svm_dataflow region and control logic region. There is a iteration loop out of these two regions mentioned above. Its break condition is determined by control logic and maximum iteration config read from DDR.

.. image:: /images/svm_train.png
   :alt: svm_train Top Structure
   :align: center


Config description
=======================

The sample and config input share one 512-bit port. Config is stored in the first 512 bit. The weight vector is output by a 512-bit port, which will be aligned to 512 bit boundaries.
Config's details are documented in the following table:

+----------+---------+-----------+----------+-----------+---------------+---------------+-----------------+
| 511-448  | 447-384 | 383-320   | 319-256  | 255-192   | 191-128       | 127-64        | 63-0            |
+==========+=========+===========+==========+===========+===============+===============+=================+
| columns  | offset  | tolerence | reg_para | step_size | sample_number | max_iteration | features_number |
+----------+---------+-----------+----------+-----------+---------------+---------------+-----------------+


Resource Utilization
====================

The hardware resource utilization of svm_train (four kernels in four SLRs) is shown in the following table (work as 276 MHz).

+--------+---------------+--------------+----------+--------+------+------+
|  LUT   | LUT as Memory | LUT as Logic | Register | BRAM36 | URAM | DSP  |
+--------+---------------+--------------+----------+--------+------+------+
| 617382 |    60868      |    556514    |  853134  |  1007  |  32  | 1256 |
+--------+---------------+--------------+----------+--------+------+------+


Benchmark Result on the Board
=========================

Meanwhile, the benchmark results at a 276 MHz frequency on an AMD Alveo™ U250 board with a 2019.2 shell are shown as follows:

+---------+---------+---------+----------+------------+-------------------+-------------------+--------------------+--------------------+--------------------+------------+
| Dataset | Samples | Classes | Features | Iterations | Spark (4 Threads) | Spark (8 Threads) | Spark (16 Threads) | Spark (32 Threads) | Spark (56 Threads) | FPGA (:ms) |
+---------+---------+---------+----------+------------+-------------------+-------------------+--------------------+--------------------+--------------------+------------+
|  PUF    | 2000000 |   2     |    64    |     20     | 192548 (61.9X)    | 112903 (36.1X)    | 69806 (22.2X)      | 68548 (21.9X)      | 68080 (21.9X)      | 3078       |
+---------+---------+---------+----------+------------+-------------------+-------------------+--------------------+--------------------+--------------------+------------+
| HIGGS   | 5000000 |   2     |    28    |     100    | 2224074 (147.6X)  | 1401467 (92.9X)   | 873412 (57.9X)     | 601885 (39.8X)     | 590843 (39.1X)     | 15067      |
+---------+---------+---------+----------+------------+-------------------+-------------------+--------------------+--------------------+--------------------+------------+
