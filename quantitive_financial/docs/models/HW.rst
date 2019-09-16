
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

*******************
Hull-White Model
*******************

Overview
=========
In financial mathematics, the Hull–White model is a model of future interest rates. In its most generic formulation, it belongs to the class of no-arbitrage models that are able to fit today's term structure of interest rates. It is relatively straightforward to translate the mathematical description of the evolution of future interest rates onto a tree or lattice and so interest rate derivatives such as bermudan swaptions can be valued in the model. The first Hull–White model was described by John C. Hull and Alan White in 1990. The model is still popular in the market today (from viki).

This section mainly introduces the implementation process of short-rate and discount, which is applied in Tree Engine and FD (finite differences) Engine. They are core part for option pricing. 

Implementation
===================
As an important parts of the Tree Engine and FD Engine, the class HWModel implements the single-factor Hull-White model to calculate short-rate and discount by using continuous compounding, including 4 functions (treeShortRate, fdShortRate, discount, discountBond). Next, the implementation process is introduced.

1. Function treeShortRate: 1) The short-rates is calculated at time point :math:`t` with the duration :math:`dt` from 0 to N time point by time point. First, the variable value is calculated. In order to reduce latency, the array values16 is added to save the intermediate result. Then, the short rate is calculated based on variable value. 2) According to timepoints and tree related parameters to establish a trinomial tree structure from 0 to N time point by time point.
2. Function fdShortRate: The short-rate is calculated at time point :math:`t`.
3. Function discount: The discount is calculated at time point :math:`t` with the duration :math:`dt` that based on the short-rate.
4. Function discountBond: the discount bond is calculated at time point :math:`t` with the duration :math:`dt=T-t` that based on the short-rate.

