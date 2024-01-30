.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

******************************
Linear Regression (Predict)
******************************

Linear Least Square Regression
===============================

Linear Least Square Regression is a model to predict a sample's scalar response based on its features. Its prediction function's output is linear to samples:

.. math::
    y=\beta _{0}+\beta _{1}x_{1}+\beta _{2}x_{2}+...+\beta _{n}x_{n}

.. math::
    y= \theta ^{T}\cdot x + \beta _{0}

LASSO Regression
=================

LASSO Regression in prediction is the same with linear least square regression. Its major difference against linear least square is its training stage.

Ridge Regression
=================

Ridge Regression in prediction is the same with linear least square regression. Its major difference against linear least square is its training stage.


Implementation (inference)
===========================

Input of predict function are D streams of features, in which D is how many features that predict function process at one cycle. According to prediction model formula, to calculate final result, it has dependency issue. To achieve II = 1 to process input data, prediction comes in three stages:
- Stage 1: Compute the sum of D features multiply D weights, and name it as partSum. Later D features' processing does not depend on former D features. It could achieve II = 1.
- Stage 2: Compute the sum of partSum; you allocate a buffer whose length(L) is longer than latency of addition, and add each partSum to different position. 0th partSum will be added to 0th buff, 1st to 1st buff... The L th part Sum will be added to 0th buff. Because L is longer than the addition latency, the 0th buff has already finished the addition of 0th buffer. So Lth partSum's addition will not suffer a dependency issue,so Stage 2 could Archieve II = 1.
- Stage 3: Add L buffs, and get the final result. This part also does not have a dependency issue and could achieve II = 1.
Stage 1 to 3 are connected with streams and run dataflow. In this way, you could achieve II = 1 in total.

.. image:: /images/sl2.png
   :alt: 3 stage dataflow
   :width: 80%
   :align: center

The correctness of Linear Regression/LASSO Regression/Ridge Regression is verified by comparing results with Spark mllib. The results are identical.
