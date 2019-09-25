
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

**********************
Black-Karasinski Model
**********************

Overview
=========
In financial mathematics, the Black-Karasinski model is a mathematical model of the term structure of interest rates; see short rate model. It is a one-factor model as it describes interest rate movements as driven by a single source of randomness. It belongs to the class of no-arbitrage models, i.e. it can fit today's zero-coupon bond prices, and in its most general form, today's prices for a set of caps, floors or European Swaptions. The model was introduced by Fischer Black and Piotr Karasinski in 1991 (from Wiki).

This section mainly introduces the implementation process of short-rate and discount, which is applied in Tree Engine. They are core part for option pricing. 

Implementation
===================
As an important part of Tree Engine, the class BKModel implements the single-factor Black-Karasinski model to calculate short-rate and discount by using continuous compounding. Next, the implementation process is introduced.

1. a) The short-rate is calculated at time point :math:`t` with the duration :math:`dt` from 0 to N time point by time point by functions treeShortRate, initRate and iterRate. As the core part the function treeShortRate, The effect of the loop_rateModel is to calcutate result that satisfies the condition. For functions initRate and iterRate, their functions are similar. In order to achieve II=1, the array values16 or values3x16  is added to save the intermediate result.
   b) According to timepoints and tree related parameters to establish a trinomial tree structure from 0 to N time point by time point.
2. The discount is calculated at time point :math:`t` with the duration :math:`dt` by function discount that based on the short-rate.

