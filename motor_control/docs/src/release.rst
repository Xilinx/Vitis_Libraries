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

Motor Control Library is an open-sourced library written in C/C++ for accelerating developments of motor control applications. It now covers four algorithm-level L1 APIs including FOC, SVPWM_DUTY, PWM_GEN, and QEI. Operator-level APIs, such as Clarke transform and its inverse transform, Park transform and its inverse transform, and PID are also implemented. The use of ap_fixed data types makes the code easy to understand and further develop. A virtual motor model is provided for doing the verifications of FOC solely in the AMD Vitis |trade| environment.

2023.2
-------

In 2023.2 release, the sensor-based FOC IP has been updated by adding a new control mode 'MOD_MANUAL_TORQUE_FLUX_FIXED_ANGLE'.

2023.1
-------

The 2023.1 release covers a range of key algorithms, includes:

1. FOC: The API is for sensor based field-orientated control (FOC).The eight control modes it supports cover basic speed and torque control modes, as well as field-weakening control.
2. SVPWM_DUTY: The API is the front-end for Space Vector Pulse Width Modulation (SVPWM) to calculate ratios.
3. PWM_GEN: The API is the back-end for Space Vector Pulse Width Modulation (SVPWM) to generate output signals based on ratios.
4. QEI: The API is for quadrature encoder interface(QEI).


.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim: