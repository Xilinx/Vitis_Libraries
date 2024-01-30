.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

******************************
Logistic Regression (Predict)
******************************

Logistic Regression Classifier
===============================

Logistic Regression is a model to label a sample with integer from 0 to L - 1 in which L is class number. For a L class label, it needs L - 1 vector to calculate L - 1 margins. Its formula is shown as follows. Its prediction function's output is linear to samples:

.. math::
    margin_{i}=\beta _{i,0}+\beta _{i,1}x_{1}+\beta _{i,2}x_{2}+...+\beta _{i,n}x_{n}

Then label is decided according to L - 1 margins based on the following formula.

.. math::
    Label = \left\{\begin{matrix} 0 & if & maxMargin \leqslant  0 \\ k & if & margin_{k} = maxMargin > 0\end{matrix}\right.


Implementation (inference)
===========================

Input of predict function are D streams of features, in which D is how many features that predict function processes at one cycle. According to the prediction model formula, to calculate the final result, it has a dependency issue. To achieve II = 1 to process input data, prediction comes in four stages.
- Stage 1: Read D features in one cycles and repeat them (L - 2 + K) / K times.
- Stage 2: Compute the sum of D features multiply D weights, and name it as partSum. Later D features' processing does not depend on former D features. It could achieve II = 1.
-0 Stage 3: Compute the sum of partSum; you allocate a buffer whose length(L) is longer than latency of the addition, and add each partSum to different position. 0th partSum will be add to 0th buff, 1st to 1st buff... The L th part Sum will be added to 0th buff. Because L is longer than the addition latency, the 0th buff has already finished addition of 0th buffer. So Lth partSum's addition will not suffer a dependency issue. So Part 3 could Archieve II = 1.
- Stage 4: Add L buffs, and get the final result. This part also does not have dependency issue and could achieve II = 1.
Stage 1 to 4 are connected with streams and run dataflow.

.. image:: /images/sl2_1.png
   :alt: 4 stages dataflow
   :width: 80%
   :align: center

The correctness of Logistic Regression is verified by comparing results with Spark mllib. The results are identical.
