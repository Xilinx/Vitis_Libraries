.. 
   Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
   SPDX-License-Identifier: X11
   
   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:
   
   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.
   
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   IN THE SOFTWARE.
   
   Except as contained in this notice, the name of Advanced Micro Devices 
   shall not be used in advertising or otherwise to promote the sale,
   use or other dealings in this Software without prior written authorization 
   from Advanced Micro Devices, Inc.

.. _release_note:

Release Note
============

.. toctree::
   :hidden:
   :maxdepth: 1

Motor Control Library is an open-sourced library written in C/C++ for accelerating developments of motor control applications. It now covers 4 algorithm-level L1 APIs including FOC, SVPWM_DUTY, PWM_GEN and QEI. Operator-level APIs, such as Clarke transform and its inverse transform, Park transform and its inverse transform and PID are also implemented. The use of ap_fixed data types makes the code easy to understand and further develop. A virtual motor model is provided for doing the verifications of FOC solely in the Vitis environment.

2023.1
-------

The 2023.1 release covers a range of key algorithms, includes:

1. FOC: the API is for sensor based field-orientated control (FOC).The eight control modes it supports cover basic speed and torque control modes, as well as field-weakning control.
2. SVPWM_DUTY: the API is the front-end for Space Vector Pulse Width Modulation (SVPWM) to calculate ratios.
3. PWM_GEN: the API is the back-end for Space Vector Pulse Width Modulation (SVPWM) to generate output signals based on ratios.
4. QEI: the API is for quadrature encoder interface(QEI).

Additional updates for 2023.1 happened, including:

1. The sensor-based FOC IP has been updated by adding a new control mode ‘MOD_MANUAL_TORQUE_FLUX_FIXED_ANGLE’. 
2. The QEI IP has bee updated for output signed RPM value.
3. The constrains for resource usage have been adjusted for better timing convergences on varies platforms.
