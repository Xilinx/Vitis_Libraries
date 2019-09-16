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
MCEuropeanEngine
**************************************

Usage
=====

Compute the price of European Option using the Monte Carlo Simulation.

Define the option:

.. ref-code-block:: cpp
        :class: title-code-block

        optionType = 1;
        strike = 40;
        underlying = 36;
        riskFreeRate = 0.06;
        volatility = 0.20;
        dividendYield = 0.0; 
        timeLength = 1; 
        requiredTolerance = 0.02;
        requiredSamples = 0;
        maxSamples = 0;
        timeSteps = 1;

Pre-prcoess to optimize resource:

.. ref-code-block:: cpp
        :class: title-code-block

        dt  = timeLength/timeSteps;
        var = volatility * volatility * dt;
        sqrtVar = hls::sqrt(var);
        drift = (riskFreeRate - dividendYield - 0.5 * var) * timeLength;
        discount = std::exp(-1.0 * riskFreeRate * timeLength);
        scale1 = underlying * std::exp(drift);  
        scale2 = scale1 * discount;  
        requiredTolerance /= scale2;
        strike = strike/scale1;

Call the engine:

.. ref-code-block:: cpp
        :class: title-code-block

        xf::fintech::MCEuropeanEngine<TEST_DT, xf::fintech::MT19937_InverseCumulativeNormal_RNG<double>, 1, 1>
                        (strike,
                         sqrtVar,
                         dt,
                         timeSteps, 
                         maxSamples,
                         requiredSamples,
                         requiredTolerance,
                         optionType,
                         outputs);

Post-process:

.. ref-code-block:: cpp
        :class: title-code-block

        price = outputs[0] * scale2;

Performance Profile
======================
