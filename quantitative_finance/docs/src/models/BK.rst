
.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Model, finance, Black-Karasinski
   :description: The Black-Karasinski model is a mathematical model of the term structure of interest rates. 
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials



**********************
Black-Karasinski Model
**********************

Overview
=========
In financial mathematics, the Black-Karasinski model is a mathematical model of the term structure of interest rates; see short rate model. It is a one-factor model as it describes interest rate movements as driven by a single source of randomness. It belongs to the class of no-arbitrage models, that is, it can fit today's zero-coupon bond prices, and in its most general form, today's prices for a set of caps, floors or European Swaptions. The model was introduced by Fischer Black and Piotr Karasinski in 1991 (from Wiki).

Implementation
===================
This section mainly introduces the implementation process of short-rate and discount, which is core part of option pricing, and applied in Tree Engine.

As a critical part of Tree Engine, the class :math:`BKModel` implements the single-factor Black-Karasinski model to calculate short-rate and discount by using continuous compounding. The implementation process is introduced as follows:

1. a) The short-rate is calculated at time point :math:`t` with the duration :math:`dt` from 0 to N point-by-point by functions treeShortRate, initRate and iterRate. As the core part of the treeShortRate, the outer loop_rateModel is used to ensure the results under pre-specified tolerance. For the internal functions, the functionality of initRate and iterRate is similar with each other, but initRate can produce 3 intermediate results while the iterRate gives only one per iteration. To achieve initiation interval (II)=1, the array values16 is added to store the intermediate results. Then an addition tree is performed subsequently for the whole process.
   b) For implementing the generic Tree framework, the :math:`state\_price` calculating process is moved from Tree Lattice to this Model.
2. The discount is calculated at time point :math:`t` with the duration :math:`dt` based on the short-rate.

