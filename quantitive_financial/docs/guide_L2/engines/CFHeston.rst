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


*************************************
Internal Design of Closed Form Heston
*************************************

Overview
========

Heston models the dynamics of a financial market; it adds a stochastic volatility aspect to the Black-Scholes model.

The Heston Partial Differential Equation (PDE) for the price, and the closed-form equations for its solution, are given in the "Models" and "Methods" sections here for the Heston Model.


Design Structure
================

There are two layers to the kernel; the engine itself and the IO wrapper.

The Engine (hcf_engine.h)
=========================

The engine performs a single Heston Closed Form solution for a European Call Option.

It is templated to generate either a floating point (Float-32) kernel or a double (Float-64) kernel. The double kernel will be more accurate but takes up more FPGA resource. The code contains some HLS directives which ensure the design is pipelined and parallelised where suitable in order to increase its performance. It uses complex number arithmetic which is implemented by wrapper functions that in turn use the HLS math library.

The integration is performed using the trapezoidal rule with a configurable dw (the integration time step) and Wmax (the upper limit of the integral). These parameters are configurable at run time and are passed to the kernel along with the Heston parameters.


IO Wrapper (hcf_kernel.cpp)
===========================

The wrapper takes as input an array of parameter data and iterates through the array calling the Engine for each entry. The results are returned as an array. This is done in order to make efficient use of DMA to and from the FPGA; one batch data transaction being much faster than multiple single transactions. The data is first read from global memory into local memory, then processed and finally the output data copied from local memory back to global memory. This is done because the extra time required by the copies is more than compensated for by the speedup of the Engine accessing local memory.


Resource Utilization
====================

The floating point kernel Area Information:

:FF:         195663  
:LUT:        211957
:DSP:        1149
:BRAM:       28
:URAM:       0

The double kernel Area Information:

:FF:         644664  
:LUT:        654672
:DSP:        3849
:BRAM:       520
:URAM:       0


Throughput
==========

The theoretical throughput depends on different factors. A floating point kernel will be faster than a double kernel. A smaller dw and larger Wmax will provide more accurate results but will decrease throughput. The kernel has been pipelined in order to increase the throughput when a large number of inputs is to be processed.

Throughput is composed of three processes; transferring data to the FPGA, running the computations and transferring the results back from the FPGA. The demo contains options to measure timings as described in the README.md file.

As an example, processing a batch of 1000 call calculations with a floating point kernel with dw = 0.5 and Wmax = 200 breaks down as follows:

Time to transfer data = 0.26ms

Time for 1000 calculations = 14.4ms (equates to 14.4us per calculation)

Time to transfer results = 0.18ms






.. toctree::
   :maxdepth: 1
