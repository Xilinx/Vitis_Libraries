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

Motor Control Library is an open-sourced library written in C/C++ for accelerating Motor control. It now covers a level of acceleration: the module level(L1) and the pre-defined kernel level(L2). Currently 3 kinds of algorithms are accelerated. 

2023.1
-------

The 2023.1 release provides a range of algorithms, includes:

1. FOC: 1 L1 API is provided for sensor based field-orientated control (FOC), Argument reg and status reg to help to control the system.
2. SVPWM_DUTY: 1 L1 API is provided for Space Vector Pulse Width Modulation (SVPWM), Argument reg and status reg to help to control the system.
3. PWM_GEN: 1 L1 API is provided for Space Vector Pulse Width Modulation (SVPWM), Argument reg and status reg to help to control the system.
4. QEI: 1 L1 API is provided for the interface to incremental encoders for obtaining mechanical position data, Argument reg and status reg to help to control the system.
5. FOC_sensorless: 1 L1 API is provided for sensor-less field-orientated control (FOC), Argument reg and status reg to help to control the system.
