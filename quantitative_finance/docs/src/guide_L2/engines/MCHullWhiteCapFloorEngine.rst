.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Monte Carlo, Hull-White, interest rate, pricing, engine, MCHullWhiteCapFloorEngine
   :description: Using the Monte Carlo Simulation to estimate the value of Cap/Floor Contract. Here, we use Hull-White Model to describe short-term interest rate movement.    
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials



********************************************
Internal Design of MCHullWhiteCapFloorEngine
********************************************

Overview
=========

Using the Monte Carlo Simulation to estimate the value of Cap/Floor Contract. Here, Hull-White Model is used to describe short-term interest rate movement.

Interest rate cap is a contract in which the buyer receives payments at the end of each period in which the interest rate exceeds the agreed strike price. Interest rate floor is a contract in which the buyer receives payments at the end of each period in which the interest rate is below the agreed strike price.

The payoff is calculated as follows:

  payoff of interest cap = :math:`max(L-K, 0) * N * \alpha`

  payoff of interest floor = :math:`max(K-L, 0) * N * \alpha`

Where :math:`K` is the strike interest rate and :math:`L` is the actual interest rate that apply to this period. :math: `N` is nomial value of cap/floor contract and :math: `\alpha` is time fraction corresponding to this period.

Implementation 
===============

In Monte Carlo Framework, the path generator is specified with Hull-White model. For path pricer, it fetches the interest sequence from the input stream, calculates the payoff based on above formula, and discounts it to time 0 for cap/floor price.




