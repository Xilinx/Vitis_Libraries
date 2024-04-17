
.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: fintech, Stochastic, StochasticProcess1D
   :description: Stochastic process uses a given point and a time step to calculate the expectation and variance.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


*********************
Stochastic Process
*********************

Overview
=========
`Stochastic process` uses the Random Number Generator(RNG). It uses a given point :math:`(t,x)` and a time step :math:`\Delta t` to calculate the expectation and variance. The class `StochasticProcess1D` is 1-dimensional stochastic process, and it cooperates with Cox-Ingersoll-Ross and Extended Cox-Ingersoll-Ross Models.
The stochastic process can be described as

.. math::
  dx_{t}=\mu(t,x_{t})dt+\sigma(t,x_{t})dW_{t}

Implementation
===================
The implementation of `StochasticProcess1D` is comprised by a few methods. The implementation is introduced as follows:

1. `init`: Initialization function used to set up the arguments as below:

    a)speed, the spreads on interest rates;

    b)vola, the overall level of volatility;

    c)x0, the initial value of level;

    d)level, the width of fluctuation on interest rates.

2. `expectation`: The expectation method returns the expectation of the process at time :math:`E(x_{t_{0}+\Delta t}|x_{t_{0}}=x_{0})`. 

3. `variance`: The variance method returns the variance :math:`V(x_{t_{0}+\Delta t}|x_{t_{0}}=x_{0})` of the process during a time interval :math:`\Delta t` according to the given volatility.

