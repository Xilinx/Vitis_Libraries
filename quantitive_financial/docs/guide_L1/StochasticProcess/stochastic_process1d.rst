
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
StochasticProcess1D 
*******************

Overview
=========
Stochastic processes is drived by using RNGs, for example Monte Carlo, generates random numbers. It starts a point :math:`(t,x)` and a time step :math:`\Delta t` and return a discretization of the process drift and diffusion. StochasticProcess1D is 1-dimensional stochastic process, whose factor of having one direction.  
The StochasticProcess1D describes by

.. math::
  dx_{t}=\mu(t,x_{t})dt+\sigma(t,x_{t})dW_{t}

Implementation
===================
The implementation of StochasticProcess1D defines a few inspectors. The expectation method returns the expectation values of the process variables at time :math:`E(x_{t_{0}+\Delta t}|x_{t_{0}}=x_{0})`. The variance method returns the variance :math:`V(x_{t_{0}+\Delta t}|x_{t_{0}}=x_{0})` of the process after a time interval :math:`\Delta t` according to the given discretization. 

