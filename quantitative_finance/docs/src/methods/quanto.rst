.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Quanto, Closed-Form, Black Scholes Merton
   :description: The Quanto option gives the buyer the right to sell a foreign asset in a foreign currency at the exercise date and receive the proceeds in his domestic currency. 
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials



***************************
Quanto Closed-Form Solution
***************************

.. toctree::
   :maxdepth: 1

Overview
========

Quanto option gives the buyer the right to sell a foreign asset in a foreign currency at the exercise date and receive the proceeds in his domestic currency.

The operation of Quanto option explains what it is:

The buyer of a call Quanto agrees with the seller a foreign target asset, a future strike price for that asset in the foreign currency, an exercise date and an agreed fixed exchange rate for the exercise.

At the exercise date, if the option is in the money, the payoff (current foreign currency asset price - foreign currency strike price) is converted to the domestic currency at the agreed fixed exchange rate and delivered to the Option buyer. 

Quanto Options can be modelled using a modified Black Scholes Merton solution as described in [HAUG]_, with suitable parameter conversions as described below:

.. math::
   c = E\left[S.\mathrm{e}^{\left(-r_f-r_d-q-\rho.\sigma_S.\sigma_E\right)T}N(d_1) - K.\mathrm{e}^{-r_dT}N(d_2)\right]

.. math::
    d_1 = \frac{ln\left(\frac{S}{K}\right)+\left(r_f - q - \rho.\sigma_S.\sigma_E + \frac{\sigma_S^2}{2}\right)T}{\sigma_S\sqrt{T}}

.. math::
    d_2 = \frac{ln\left(\frac{S}{K}\right)+\left(r_f - q - \rho.\sigma_S.\sigma_E - \frac{\sigma_S^2}{2}\right)T}{\sigma_S\sqrt{T}} = d_1 - \sigma_S\sqrt{T}

where:

c = call price in domestic currency

S = underlying asset price in a foreign currency

K = strike price in foreign currency

:math:`r_d` = domestic interest rate

:math:`r_f` = foreign interest rate

:math:`N()` = cumulative standard normal distribution function

T = time to maturity

q = dividend yeild of the foreign asset

E = spot exchange rate, foreign currency units per domestic currency unit

:math:`\sigma_S` = foreign asset volatility

:math:`\sigma_E` = domestic exchange rate volatility

:math:`\rho` = correlation between asset and domestic exchange rate

When Comparing with the equations in Black Scholes Merton Closed-Form solution, as shown:

.. math::
   c = S.\mathrm{e}^{qT}N(d_1) - K.\mathrm{e}^{-rT}N(d_2)

.. math::
    d_1 = \frac{ln\left(\frac{S}{K}\right)+\left(r - q + \frac{\sigma^2}{2}\right)T}{\sigma\sqrt{T}}

.. math::
    d_2 = \frac{ln\left(\frac{S}{K}\right)+\left(r - q - \frac{\sigma^2}{2}\right)T}{\sigma\sqrt{T}} = d_1 - \sigma\sqrt{T}

it can be seen that the following substitutions can be set to make the solution equal to the Black Scholes Merton solution:

c -> c.E

r -> :math:`r_d`

:math:`\sigma` -> :math:`\sigma_S`

q -> -:math:`r_f` + r + q + :math:`\rho.\sigma_S.\sigma_E`

References
==========

.. [HAUG] Haug, E.G. (2007): The Complete Guide to Option Pricing Formulas, §5.16.2, p228

