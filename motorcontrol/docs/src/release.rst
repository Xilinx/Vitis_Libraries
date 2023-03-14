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

.. _release_note:

Release Note
============

.. toctree::
   :hidden:
   :maxdepth: 1

Motor Control Library is an open-sourced library written in C/C++ for accelerating Motor control. It now covers a level of acceleration: the module level(L1) and the pre-defined kernel level(L2). Currently 3 kinds of algorithms are accelerated. 

2022.2
-------

The 2022.2 release provides a range of algorithms, includes:

1. FOC: 1 L1 API is provided for sensor based field-orientated control (FOC), Argument reg and status reg to help to control the system.
2. SVPWM_DUTY: 1 L1 API is provided for Space Vector Pulse Width Modulation (SVPWM), Argument reg and status reg to help to control the system.
3. PWM_GEN: 1 L1 API is provided for Space Vector Pulse Width Modulation (SVPWM), Argument reg and status reg to help to control the system.
4. QEI: 1 L1 API is provided for the interface to incremental encoders for obtaining mechanical position data, Argument reg and status reg to help to control the system.
5. FOC_sensorless: 1 L1 API is provided for sensor-less field-orientated control (FOC), Argument reg and status reg to help to control the system.