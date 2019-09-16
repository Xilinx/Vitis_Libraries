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

**************************************
MCAsianEngine
**************************************

Compute the price of Asian Option using the Monte Carlo Simulation. 

Examples
========

In this example, the average price on the lifetime of option is used as the settlement price at the maturity date.


Define the option:

.. ref-code-block:: cpp
        :class: title-code-block

        McAsianAPEngine:
        optionType = 1;
        strike = 87;//40;
        underlying = 90;//36;
        riskFreeRate = 0.025;//0.06;
        volatility = 0.13;//0.20;
        dividendYield = 0.06;//0.0;
        timeLength = 11.0/12.0;
        requiredTolerance = 0.02;//0.02;
        requiredSamples = 0;
        maxSamples = 0;
        timeSteps = 12;

Pre-prcoess to optimize resource:

.. ref-code-block:: cpp
        :class: title-code-block
        
        McAsianAPEngine:
        dt = timeLength/timeSteps;
        var = volatility * volatility * dt;
        sqrtVar = hls::sqrt(var);
        drift = (riskFreeRate - dividendYield) * dt - 0.5 * var;
        discount = hls::exp(-1 * riskFreeRate * timeLength);
        scale1 = underlying;
        scale2 = scale1 * discount;  
        requiredTolerance /= scale1;
        strike = strike/scale1;

Call the engine:

.. ref-code-block:: cpp
        :class: title-code-block

	xf::fintech::MCAsianArithmeticAPEngine<TEST_DT, UN>
                     (underlying,
                      volatility,
                      dividendYield,
                      riskFreeRate,
                      timeLength,
                      strike,
                      optionType,
                      * seed,
                      output, 
                      requiredTolerance,
                      requiredSamples,
                      timeSteps,
                      maxSamples);





