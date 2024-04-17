.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: European, pricing, engine, MCEuropeanEngine
   :description: The European option pricing engine uses Monte Carlo Simulation to estimate the value of European Option. Here, it is assumed that the process of asset pricing applies to Black-Scholes process.   
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials



**************************************************
Internal Design of European Option Pricing Engine 
**************************************************

Overview
=========

The European option pricing engine uses Monte Carlo Simulation to estimate the value of European Option. Here, we assume the process of asset pricing applies to Black-Scholes process. 

European option is a kind of vanilla option and not path dependent. The option has the right
but not the obligation, to be exercised at the maturity time. That is to say, the payoff
is only related to the price of the underlying asset at the maturity time.

The payoff is calculated as follows:

  payoff of Call Option = :math:`max(S-K,0)`

  payoff of Put Option = :math:`max(K-S,0)`

Where :math:`K` is the strike value and :math:`S` is the spot price of underlying asset at maturity time.

Implementation 
===============

In Monte Carlo Framework, the path generator is specified with Black-Scholes. For path pricer, it fetches the `logS` from the input stream, calculates the payoff based on above formula and discounts it to time 0 for option price.

