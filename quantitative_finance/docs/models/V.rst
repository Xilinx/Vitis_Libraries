
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

************************
Vasicek Model
************************

Overview
=========
In financial mathematics, the Vasicek Model is a mathematical model describing the evolution of interest rates. It is a type of one-factor short rate model as it describes interest rate movements as driven by only one source of market risk. The model can be used in the valuation of interest rate derivatives, and has also been adapted for credit markets (from Wiki).

As the base of the Hull-White model, the Vasicek Model is a outdated model. This section mainly introduces the implementation process of short-rate and discount, which is applied in Tree Engine.They are core part for option pricing. 

Implementation
===================
As an important part of Tree Engine, the class HWModel implements the single-factor Vasicek model to calculate short-rate and discount by using continuous compounding. Next, the implementation process is introduced.

1. a) Since the short-rate at the current time point is independent of the short-rate at the previous time point, the short-rate is not calculated independently. 
   b) According to time points and tree related parameters to establish a trinomial tree structure from 0 to N time point by time point.
2. The discount is calculated at time point :math:`t` with the duration :math:`dt` by function discount.

