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

*******************************************
Internal Design of MCEuropeanHestonEngine
*******************************************


Overview
========

The European option is an option that can only be exercised at the expiration date. 
Heston Model is the most classic model for stock price. 
This engine uses a large number of random samples to simulate stock price's dynamic based on Heston Model, 
then calculates the value of option which use this stock as underlying asset.

Implementation
==============

This engine uses the framework of Monte Carlo Simulation in L1. 
This engine has a template argument to setup unroll number of Monte Carlo Module(MCM). 
For a single MCM, it has two normal distributed random number generators as input, to simulate random walks of stock price and its volatility. 
Two RNGs will send a stream of random numbers to Heston PathGenerator to calculate the stock price at each time steps. 
Price of stock will be sent to pricer to calculate option's value of each path. 
After certain number of paths have been generated, accumulator will determine whether the value of options has become "stable" according to its variance. 
If not stable, MCM will move to the next round of calculation to generate more path. 
If stable, it will stop and output the average of option values as the final result.

.. image:: /images/mcht.png
   :alt: Diagram of MCEuropeanHestonEngine
   :width: 80%
   :align: center

Variations 
==========

In this release we provide five variations of Heston Model implementation, 
including kDTFullTruncation, kDTPartialTruncation, kDTReflection, kDTQuadraticExponential and kDTQuadraticExponentialMartingale. 
The first three is relatively simple dealing with negative volatility. 
kDTQuadraticExponential and kDTQuadraticExponential Martingale use better approximation method to get better precision result, and take more resource. 

Optimization comes in two parts. 
First and the most is optimization of L1 functions. 
The second is we save one call of cumulative distribution function since we can get this value directly from RNGs. 
The second part may not hold when dealing with multiple underlying assets in the future because it may lose direct link of Gaussian random number and its corresponding uniform random number.
