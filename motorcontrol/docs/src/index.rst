.. 
   Copyright 2022 Xilinx, Inc.
  
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
  
       http://www.apache.org/licenses/LICENSE-2.0
  
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.


.. Project documentation master file, created by
   sphinx-quickstart on Tue Oct 30 18:39:21 2018.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Vitis Motor Control Library
=============================

Motor Control Library is an open-sourced library written in C/C++ for accelerating Motor control. It now covers a level of acceleration: the module level(L1). Currently 5 kinds of algorithms are accelerated, including FOC, SVPWM__DUTY, PWM_GEN, QEI and FOC_sensorless:


- FOC: 1 L1 API is provided for sensor based field-orientated control (FOC), Argument reg and status reg to help to control the system.
- SVPWM_DUTY: 1 L1 API is provided for Space Vector Pulse Width Modulation (SVPWM), Argument reg and status reg to help to control the system.
- PWM_GEN: 1 L1 API is provided for Space Vector Pulse Width Modulation (SVPWM), Argument reg and status reg to help to control the system.
- QEI: 1 L1 API is provided for quadrature encoder interface(QEI), Argument reg and status reg to help to control the system.
- FOC_sensorless: 1 L1 API is provided for sensorless field-orientated control (FOC),  Argument reg and status reg to help to control the system.


.. toctree::
   :caption: Introduction
   :maxdepth: 1

   Overview <overview.rst>
   Release Note <release.rst>
   Vitis Motor Control Library Tutorial <tutorial.rst>

.. toctree::
   :caption: L1 User Guide
   :maxdepth: 3

   API Document <guide_L1/api.rst>

.. toctree::
   :maxdepth: 2

   Design Internals <guide_L1/internals.rst>

.. toctree::
..    :caption: L2 User Guide
..    :maxdepth: 3

..    API Document <guide_L2/api.rst>

.. toctree::
..    :maxdepth: 2

..    Design Internals <guide_L2/internals.rst>

.. toctree::
   :caption: Benchmark 
   :maxdepth: 1 

   Benchmark <benchmark.rst>

Indices and tables
------------------

* :ref:`genindex`
* :ref:`search`
