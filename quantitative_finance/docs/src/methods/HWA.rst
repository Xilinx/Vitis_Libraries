.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

****************************************
Hull White Analytic Closed-Form Solution
****************************************

.. toctree::
   :maxdepth: 1

Overview
========

In financial mathematics, the Hull-White model is a model of future interest rates and is an extension the Vasicek model.

Its an no-arbitrage model that is able to fit todays term structure of interest rates.

It assumes that the short-term rate is normally distributed and subject to mean reversion.


The stochastic differential equation describing Hull-White is:

.. math::
        \delta{r} = [\theta(t) - ar]\delta{t} + \sigma\delta{z}

These input parameters are:

:math:`\delta r` - is the change in the short-term interest rate over a small interval

:math:`\theta (t)` - is a function of time determining the average direction in which r moves (derived from yield curve)

:math:`a` - the mean reversion

:math:`r` - the short-term interest rate

:math:`\delta t` - a small change in time

:math:`\sigma` - the volatility

:math:`\delta z` - is a Wiener (Random) process
