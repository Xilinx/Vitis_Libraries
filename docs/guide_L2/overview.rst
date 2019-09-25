..
   Copyright 2019 Xilinx, Inc.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

***********************
Pricing Engine Overview
***********************

Vitis Quantitative Finance Library 1.0 provides 12 pricing engines to calculate price for the following options.

* European Option
* American Option
* Asian Option
* Barrier Option
* Digital Option
* Cliquet Option

Additionally, the following options have 2 Closed-Form solution engines; the Black-Scholes-Merton model and the Heston model.

* European Option

There is also a Binomial Tree (Cox-Ross-Rubinstein) engine that will calculate prices for:

* European Option
* American Option


The main feature for each pricing engines is as the following table.

+-----------------------+-----------+-------------+----------------+
|Pricing Engines        |Option     |Model        |Solution Method |
+-----------------------+-----------+-------------+----------------+
|MCEuropeanEngine       |European   |Black-Scholes| Monte Carlo    |
+-----------------------+-----------+             +                +
|MCAsianAPEngine        |Asian      |             |                |
+-----------------------+           +             +                +
|MCAsianGPEngine        |           |             |                |
+-----------------------+           +             +                +
|MCAsianASEngine        |           |             |                |
+-----------------------+-----------+             +                +
|MCCliquetEngine        |Cliquet    |             |                |
+-----------------------+-----------+             +                +
|MCDigitalEngine        |Digital    |             |                |
+-----------------------+-----------+             +                +
|MCBarrierEngine        |Barrier    |             |                |
+-----------------------+           +             +                +
|MCBarrierNoBiasEngine  |           |             |                |
+-----------------------+-----------+             +                +
|MCAmericanEngine       |American   |             |                |
+-----------------------+-----------+-------------+                +
|MCEuropeanHestonEngine |European   |Heston       |                |
+-----------------------+           +             +                +
|MCMultiAssetEuropean/  |European   |             |                |
|HestonEngine           |           |             |                |
+-----------------------+-----------+-------------+----------------+
|FdHullWhiteEgnine      |Bermudan   |Hull-White   |finite-differen/|
|                       |           |             |ce methods      |
+-----------------------+-----------+-------------+----------------+
|CFBlackScholes         |European   |Black-Scholes| Closed Form    |
+-----------------------+-----------+-------------+                +
|CFHeston               |European   |Heston       |                |
+-----------------------+-----------+-------------+----------------+
|BTCRR                  |European   |Cox-Ross-    | Binomial Tree  |
|                       |American   | Rubinstein  |                |
+-----------------------+-----------+-------------+----------------+
