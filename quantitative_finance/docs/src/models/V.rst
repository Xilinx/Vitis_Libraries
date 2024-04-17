
.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Model, finance, Vasicek, VModel
   :description: The Vasicek Model is a mathematical model describing the evolution of interest rates.  
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

************************
Vasicek Model
************************

Overview
=========
In financial mathematics, the Vasicek Model is a mathematical model describing the evolution of interest rates. It is a type of one-factor short rate model as it describes interest rate movements as driven by only one source of market risk. The model can be used in the valuation of interest rate derivatives, and has also been adapted for credit markets (from Wiki).

As widely-used of the Hull-White model, the Vasicek Model is an outdated model.

Implementation
===================
This section mainly introduces the implementation process of short-rate and discount, which is applied in Tree Engine.
As a key part of Tree Engine, the class :math:`VModel` implements the single-factor Vasicek model to calculate short-rate and discount by utilizing continuous compounding. Here, the implementation process is introduced.

1. a) Since the short-rate at the current time point is independent from the short-rate at the previous time point, there is no need to calculate the short-rate in this module.
   b) For implementing the generic Tree framework, this model only performs the calculation of some trinomial tree related parameters.
2. The discount is calculated at time point :math:`t` with the duration :math:`dt` based on the short-rate.

