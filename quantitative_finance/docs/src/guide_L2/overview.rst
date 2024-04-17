..
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Vitis Quantitative Finance Library, Black-Scholes-Merton, Heston, European, American, Asian, Barrier, Digital, Cliquet, Binomial Tree, Cox-Ross-Rubinstein, Hull-White, Black-Scholes, Monte Carlo
   :description: Vitis quantitative finance library provides pricing engines to calculate price. 
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


***********************
Pricing Engine Overview
***********************

AMD Vitis |trade| Quantitative Finance Library 1.0 provides 12 pricing engines to calculate price for the following options.

* European Option
* American Option
* Asian Option
* Barrier Option
* Digital Option
* Cliquet Option

Additionally, the following options have two Closed-Form solution engines; the Black-Scholes-Merton model and the Heston model.

* European Option

There is also a Binomial Tree (Cox-Ross-Rubinstein) engine that calculates prices for:

* European Option
* American Option


The main feature for each pricing engines is as the following table.

+-------------------------+--------------------+----------------------------+--------------------------+
|Pricing Engines          |Option              |Model                       |Solution Method           |
+-------------------------+--------------------+----------------------------+--------------------------+
|MCEuropeanEngine         |European            |Black-Scholes               | Monte Carlo              |
+-------------------------+--------------------+                            +                          +
|MCAsianAPEngine          |Asian               |                            |                          |
+-------------------------+                    +                            +                          +
|MCAsianGPEngine          |                    |                            |                          |
+-------------------------+                    +                            +                          +
|MCAsianASEngine          |                    |                            |                          |
+-------------------------+--------------------+                            +                          +
|MCCliquetEngine          |Cliquet             |                            |                          |
+-------------------------+--------------------+                            +                          +
|MCDigitalEngine          |Digital             |                            |                          |
+-------------------------+--------------------+                            +                          +
|MCBarrierEngine          |Barrier             |                            |                          |
+-------------------------+                    +                            +                          +
|MCBarrierNoBiasEngine    |                    |                            |                          |
+-------------------------+--------------------+                            +                          +
|MCAmericanEngine         |American            |                            |                          |
+-------------------------+--------------------+----------------------------+                          +
|MCEuropeanHestonEngine   |European            |Heston                      |                          |
+-------------------------+                    +                            +                          +
|MCMultiAssetEuropean/    |                    |                            |                          |
|HestonEngine             |                    |                            |                          |
+-------------------------+--------------------+----------------------------+--------------------------+
|CFBlackScholes           |European            |Black-Scholes               | Closed Form              |
+-------------------------+--------------------+----------------------------+                          +
|CFBlack76                |European            |Black 76                    |                          |
+-------------------------+--------------------+----------------------------+                          +
|CFHeston                 |European            |Heston                      |                          |
+-------------------------+--------------------+----------------------------+--------------------------+
|BTCRR                    |European            |Cox-Ross-Rubinstein         | Binomial Tree            |
|                         |American            |                            |                          |
+-------------------------+--------------------+----------------------------+--------------------------+
|FdHullWhiteEngine        |Swaption            |Hull-White                  |finite-difference methods |
+-------------------------+                    +----------------------------+                          +
|FdG2SwaptionEngine       |                    |Two-additive factor Gaussian|                          |
+-------------------------+                    +----------------+-----------+--------------------------+
|treeSwaptionEngine       |                    |Hull-White                  |Trinomial Tree            |
|                         |                    |Black-Barasinski            |                          |
|                         |                    |Cox-Ingersoll-Ross          |                          |
|                         |                    |Extended Cox-Ingersoll-Ross |                          |
|                         |                    |Vasicek                     |                          |
|                         |                    |Two-additive factor Gaussian|                          |
+-------------------------+--------------------+----------------------------+                          +
|treeSwapEngine           |Swap                |Hull-White                  |Trinomial Tree            |
|                         |                    |Black-Barasinski            |                          |
|                         |                    |Cox-Ingersoll-Ross          |                          |
|                         |                    |Extended Cox-Ingersoll-Ross |                          |
|                         |                    |Vasicek                     |                          |
|                         |                    |Two-additive factor Gaussian|                          |
+-------------------------+--------------------+----------------------------+                          +
|treeCapFloorEngine       |Cap/Floor           |Hull-White                  |Trinomial Tree            |
|                         |                    |Black-Barasinski            |                          |
|                         |                    |Cox-Ingersoll-Ross          |                          |
|                         |                    |Extended Cox-Ingersoll-Ross |                          |
|                         |                    |Vasicek                     |                          |
|                         |                    |Two-additive factor Gaussian|                          |
+-------------------------+--------------------+----------------------------+                          +
|treeCallableBondEngine   |Callable Bond       |Hull-White                  |Trinomial Tree            |
|                         |                    |Black-Barasinski            |                          |
|                         |                    |Cox-Ingersoll-Ross          |                          |
|                         |                    |Extended Cox-Ingersoll-Ross |                          |
|                         |                    |Vasicek                     |                          |
|                         |                    |Two-additive factor Gaussian|                          |
+-------------------------+--------------------+----------------------------+--------------------------+
|MCHullWhiteCapFloorEngine|Cap/Floor           |Hull-White                  |Monte Carlo               |
+-------------------------+--------------------+----------------------------+--------------------------+
|CPICapFloorEngine        |CPI Cap/Floor       | --                         |Close Form                |
+-------------------------+--------------------+----------------------------+                          +
|DiscountingBondEngine    |Discounting Bond    | --                         |                          |
+-------------------------+--------------------+----------------------------+                          +
|InflationCapFloorEngine  |Inflation Cap/Floor | --                         |                          |
+-------------------------+--------------------+----------------------------+--------------------------+
|hjmEngine                | N/A                | Heath-Jarrow-Morton        | Monte Carlo              |
+-------------------------+--------------------+----------------------------+--------------------------+
|lmmEngine                | N/A                | LIBOR Market Model (BGM)   | Monte Carlo              |
+-------------------------+--------------------+----------------------------+--------------------------+
|HWAEngine                |Bond Price          | Hull-White Analytic        | Closed Form              |
|                         |Option              |                            |                          |
|                         |Cap/Floor           |                            |                          |
+-------------------------+--------------------+----------------------------+--------------------------+
