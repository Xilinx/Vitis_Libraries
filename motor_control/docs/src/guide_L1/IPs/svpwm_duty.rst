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


******************************
Internal Design of SVPWM_DUTY
******************************

Overview
===========

This API is a front-end submodule of SVPWM function. A complete SVPWM function is composed of two components: SVPWM_DUTY and PWM_GEN. This API is the SVPWM_DUTY. It is a fully optimized implementation through the AMD HLS design methodology. It can produce the normalized duty ratios of the three-phase outputs. It has configurable parameters via AXI-Lite entries, include phase_shift, dclink_src, pwm_freq and dead_cycles, and so on.

Implemention
============

The detail algorithm implementation is illustrated as below:

.. image:: /images/API_svpwm_duty.png
   :alt: design of duty cycle
   :width: 60%
   :align: center

As it is shown in the above figures, the entire SVPWM_DUTY has five configurable parameters.

* phase_shift: flag "[-shift_0/-shift_120]". It determines whether the input voltage streams have phase shift. 

* dclink_source: flag "[-dc_adc/-dc_ref]". This flag register determines the dclink voltage source.

* pwm_freq: flag "[-pwm_fq <pwm frequency>]". This flag register provides a configurable entry for the pwm wave frequency. The pwm wave frequency is also the throughput at the output end.

* dead_cycles: flag "[-dead <dead cycles>]". The dead_cycles determines the transit time between the switch on/off. The switches pair shall not simultaneously be switching on and off, in terms of the danger of overloaded transient currents on the bridge. It might incur the systematic turbulence and cause serious problem. The default value of dead_cycles is 10.  

* sampling ii: flag "[-ii <sampling II>]". The ii (initiation interval) determines the sampling rate of the input. The default value is 1. 


Profiling 
============

The Post-Synthesis Resource usage is shown in the following table.   

.. table:: Table 1 Post-Synthesis Resource usage
    :align: center

    +------------------+------------+----------+-----------+----------+---------+--------+--------+
    |       Name       |    SLICE   |    LUT   |     FF    |   BRAM   |   URAM  |   DSP  |   SRL  |
    +------------------+------------+----------+-----------+----------+---------+--------+--------+
    |    SVPWM_DUTY    |      0     |   1886   |    2346   |     0    |     0   |    3   |    0   |
    +------------------+------------+----------+-----------+----------+---------+--------+--------+


.. toctree::
    :maxdepth: 1
