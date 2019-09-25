
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
OrnsteinUhlenbeckProcess  
************************

Overview
=========
Ornstein-Uhlenbeck process is also a stochastic processe which derived by using RNGs. It starts a point :math:`(t,x)` and a time step :math:`\Delta t` and return a discretization of the process drift and diffusion. The Ornstein-Uhlenbeck process is a simple one of stochastic processes, whose feature of interest is that its mean-reverting drift term :math:`\theta(\mu-x)` and its constant diffusion term :math:`\sigma` can be integrated exactly.  
The Ornstein-Uhlenbeck process describes by

.. math::
   dx=a(r-x_{t})dt+\sigma dW_{t}

Implementation
===================
The implementation of `OrnsteinUhlenbeckProcess` defines a few inspectors. The expectation method returns the expectation values of the process variables at time :math:`E(x_{t_{0}+\Delta t}|x_{t_{0}}=x_{0})`. The stdDeviation method returns the standard deviation :math:`S(x_{t_{0}+\Delta t}|x_{t_{0}}=x_{0})` of the process after a time interval :math:`\Delta t` according to the given discretization. The variance method returns the variance :math:`V(x_{t_{0}+\Delta t}|x_{t_{0}}=x_{0})` of the process after a time interval :math:`\Delta t` according to the given discretization. The evolve method returns the asset value after a time interval :math:`\Delta t` according to the given discretization. It returns,

.. math::
   E(x_{0},t_{0},\Delta t)+S(x_{0},t_{0},\Delta t)*\Delta w

where :math:`E` is the expectation and :math:`S` the standard deviation.

