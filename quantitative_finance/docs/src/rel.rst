.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Finance, Library, Vitis Quantitative Finance Library, fintech
   :description: Vitis Quantitative Finance library release notes.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


Release Note
============

2023.2
-----------

The L3 of this library are deprecated and removed. 

If you still use the L3 of this library, reach out to [forum](https://support.xilinx.com) for help.

2023.1
-----------

There are some known issues for this release. Use AMD Vitis |trade| 2022.2 for them.

* L2/tests/M76Engine - hw build failure on u250 platform
* L2/tests/PortfolioOptimisation - hw build failure 
* L2/tests/MCEuropeanHestonGreeksEngine - hw build failure on u50 and u200 platform
* L2/tests/MCAmericanEngineMultiKernel - hw build failure on u50 platform
* L2/tests/Quadrature - hw build failure on u200 platform
* L2/tests/MCAmericanEngine - hw build failure on u50 platform

The L3 of this library will be soon deprecated and removed. It has the following known issue.

* All L3 APIs fail when running on Ubuntu Operating System.

If you still use the L3 of this library, reach out to [forum](https://support.xilinx.com) for help.


Version 1.0
-----------

Vitis Quantitative Finance library 1.0 provides engines and primitives for the acceleration of quantitative financial applications on FPGA. It comprises two approaches to pricing:

* A family of Trinomial-Tree based pricing engines for four interest rate derivatives (including swaption, swap, cap/floor and callable bond), using six short-term interest rate models (including Hull-White, Two-additive-factor gaussian, Vasicek, Cox-Ingersoll-Ross, Extended Cox-Ingersoll-Ross and BlackKarasinski). All of these pricing engines are based on a provided generic Trinomial-Tree Framework.

* Two Finite-difference method based pricing engines for swaption, using Hull-White model and Two-additive-factor gaussian model. One Monte-Carlo based pricing engine for cap/floor, using Hull-White model, based on the Monte-Carlo simulation API provided in release 0.5. 

* Three close form pricing engine for inflation cap/floor, CPI cap/floor, and discounting bond.


Version 0.5
-----------


Vitis Quantitative Finance Library 0.5 provides engines and primitives for the acceleration of quantitative financial applications on FPGA. It comprises two approaches to pricing:

* A family of 10 Monte-Carlo based engines for six equity options (including European and American options) using Black-Scholes and Heston models. All of these pricing engines are based on a provided generic Monte Carlo simulation API, and work in parallel due to their streaming interface;

* A finite-difference PDE solver for the Heston model with supporting application code and APIs.

In addition, the library supports low-level functions, such as random number generator (RNG), singular value decomposition (SVD), and tridiagonal and pentadiagonal matrix solvers.


.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
