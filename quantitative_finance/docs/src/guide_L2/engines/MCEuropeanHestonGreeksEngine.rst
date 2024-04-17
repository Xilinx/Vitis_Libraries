.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: European, pricing, engine, MCEuropeanHestonGreeksEngine
   :description: Greeks is often used to measure the sensitivity of the derivative values to changes in the behavior of the underlying instruments from which they are derived as well as changes in the broader market, such as shifts in interest rates (from STAC-A2).    
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


************************************************
Internal Design of MCEuropeanHestonGreeksEngine
************************************************


Overview
========

Greeks is often used to measure the sensitivity of the derivative values to changes in the behavior of the underlying instruments from which they are
derived as well as changes in the broader market, such as shifts in interest rates (from STAC-A2).

The derivative value :math:`X(t)` often is affected by the initial stock price, volatility, risk-free rate and mature date.
For different sensitivity, there are different Greek alphabet. The following are some common sensitivities for option.

.. math::
        \delta:\frac{\partial X_t}{\partial S} 
.. math::
        \gamma:\frac{\partial^2 X_t}{\partial S^2} 
Both :math:`\delta` and :math:`\gamma` measures the impact of price changes on option price.

.. math::
        \theta:\frac{\partial X_t}{\partial t} 

:math:`\theta` measures the change of option price over mature time.

.. math::
        \rho:\frac{\partial X_t}{\partial r} 
:math:`\rho` measure the impact of risk-free interest rate on option price.

.. math::
        vega:\frac{\partial X_t}{\partial \sigma} 

:math:`vega` measure the impact of volatility on option price. Actually, the volatility changes over time. So, that means the option price changes
over mature time.


There are also some model specific sensitivities. For Heston model, the model parameter :math:`\kappa` (mean reversion), :math:`\theta` (long term variance) and :math:`\xi` (volatility of volatility).  
These model parameter will affect the volatility of underlying price, and then indirectly affect the option price.
To measure the sensitivity of option price to model parameter, the following model vega are defined:

.. math::
        modelVega_0 = \frac{\partial X_t}{\partial \kappa} 

.. math::
        modleVega_1 = \frac{\partial X_t}{\partial \theta}

.. math::
        modelVega_2 = \frac{\partial X_t}{\partial \xi}


Implementation
==============

The numerical method for Greeks calculation includes finite difference, pathwise, and likelihood ratio.

In this implementation, the finite difference method is used to calculate Greeks. Finite difference methods use the following differential expression to approximate 
partition derivative. 

.. math::
         \delta = \frac{X(t, S_t + h, r, \sigma) - X(t, S_t, r, \sigma)} {h}
When any of the parameter changes, MCEuropeanHestonEngine is called once to calculate the option price.


