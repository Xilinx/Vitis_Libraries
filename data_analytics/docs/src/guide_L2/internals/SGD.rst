.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

***************************************
Stochastic Gradient Descent Framework
***************************************

Stochasitc gradient descent is a method to optimize an objective function that has certain properties. It is similar to gradient descent but different in data loaded. Gradient descent uses a whole set of data, while SGD randomly choose a fraction of data from the whole set.

Because random access data in DDR will have poor efficiency on DDR bandwidth, a "Drop or Jump" table sampler is implemented. If "drop" is chosen, the sampler will continuously read data from the table, and drop a part of them. This will lead to continuously burst read of data. This is better when the fraction is not too small. If "jump" is chosen, the sampler will read a continuous bucket data, jump a few buckets, and read the next bucket. This will lead to burst read of data of a certain length and interrupted by jump. This is better when the fraction is relatively small. In such way, you could have better DDR access efficiency.
 
Each iteration, the SGD framework will compute the gradient of the current weight (and intercept if needed). Then SGD will update the weight according to the gradient. Linear Least Sqaure Regression, LASSO Regression, and Ridge Regression training share the same gradient calculation process. There are three different ways to update: Simple update, L1 update, and L2 update. They will have different training results and various desired characteristics.

Linear Least Sqaure Regression Training
========================================

Linear Least Square Regression uses a simple update:

.. math::
    \theta _{next} = \theta _{current} - currentstepsize \cdot \theta _{current}^{'}

.. math::
    currentstepsize = stepsize\cdot \frac{1}{\sqrt{iterIndex})}

LASSO Regression Training
==========================

LASSO Regression uses a L1 update:

.. math::
    \theta _{next} = \theta _{current} - currentstepsize \cdot (\theta _{current}^{'}+\gamma \cdot sig(\theta _{current}))

.. math::
    currentstepsize = stepsize\cdot \frac{1}{\sqrt{iterIndex})}

Ridge Regression Training
==========================

Ridge Regression uses a L2 update:

.. math::
    \theta _{next} = \theta _{current} - currentstepsize \cdot (\theta _{current}^{'}+\gamma \cdot \theta _{current})

.. math::
    currentstepsize = stepsize\cdot \frac{1}{\sqrt{iterIndex})}

Implementation (Training)
===========================

SGD Framework is basically two parts: gradient calculation and weight update. Gradient calculation loads data from DDR and calculates the gradient of the preset weight and is a dataflow function.
Weight update calculates the next weight based on the gradient and the chosen method and is executed after gradient calculation. The block diagram is shown as follows.

.. image:: /images/SGD.png
   :alt: 3 stage dataflow
   :width: 80%
   :align: center

The correctness of Linear Regression/LASSO Regression/Ridge Regression Training using the SGD framework is verified by comparing results with Spark mllib. The results are identical.
