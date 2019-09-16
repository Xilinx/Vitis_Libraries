
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

*********************************
Extended Cox-Ingersoll-Ross Model
*********************************

Overview
=========

In financial mathematics, by replacing the coefficients in the Cox-Ingersoll-Ross model with the time varying functions, the extended Cox-Ingersoll-Ross model is a mathematical model of the term structure of interest rates. It is a type of "one factor model" (short rate model) as it describes interest rate movements as driven by only one source of market risk. The model can be used in the valuation of interest rate derivatives (from viki).

This section mainly introduces the implementation process of short-rate and discount, which is applied in Tree Engine. They are core part for option pricing. 

Implementation
===================
As an important part of Tree Engine, the class ECIRModel implements the single-factor extended Cox-Ingersoll-Ross model to calculate short-rate and discount,by using continuous compounding. Next, the implementation process is introduced.

1. 1) The short-rate is calculated at time point :math:`t` with the duration :math:`dt` from 0 to N time point by time point by functions shortRate, initRate and iterRate. As the core part the function shortRate, The effect of the loop_rateModel is to calcutate result that satisfies the condition. For functions initRate and iterRate, their functions are similar. In order to achieve ii=1, the array values16 or values3x16  is added to save the intermediate result. 2) According to timepoints and tree related parameters to establish a trinomial tree structure from 0 to N time point by time point.
2. The discount is calculated at time point :math:`t` with the duration :math:`dt` by function discount that based on the short-rate.

