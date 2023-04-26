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
