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


***************************
Internal Design of PWM_GEN
***************************

Overview
===========

This API can been seen as the back-end submodule of SVPWM function. A complete SVPWM function is composed of two components: SVPWM_DUTY and PWM_GEN. This API is the PWM_GEN. It is a fully optimized implementation through the Xilinx HLS design methodology. It can produce the bitstreams to control the switches on/off on every branch of the bridges.  The input are the normalized duty ratios from the upstream SVPWM_DUTY. The outputs are 3 sets waveforms including the high, low and sync sets for a, b, and c signals.

Implemention
============

The algorithm implemention is illustrated as below:

.. image:: /images/API_pwm_gen.png
   :alt: design of pwm_gen
   :width: 60%
   :align: center

As it is shown in the above pictures, the entire SVPWM have three functions. PWM_GEN is a stream driven sub-module. The configurable parameters are shown below:

* phase_shift: flag "[-shift_0/-shift_120]". It determines whether the input voltage streams have phase shift.  

* pwm_freq: flag "[-pwm_fq <pwm frequency>]". This flag register provides a configurable entry for the pwm wave frequency. The pwm wave frequency is also the throughput at the output end.

* dead_cycles: flag "[-dead <dead cycles>]". The dead_cycles determines the transit time between the switch on/off. The switches pair shall not simultaneously be switching on and off, in terms of the danger of overloaded transient currents on the bridges. It might incur the systematic turbulence and cause serious problem. The default value of dead_cycles is 10. 

* stt_pwm_cycle: This parameter is to monitor the status of pwm_cycle length, which is supposed to be clk_freq/pwm_freq.

* sampling ii: flag "[-ii <sampling II>]". The ii (initiation interval) determines the sampling rate of the input. The default value is 1. 

Profiling 
============

The Post-Synthesis Resource usage is shown in the following table.   

.. table:: Table 1 Post-Synthesis Resource usage
    :align: center

    +-----------------+------------+----------+-----------+----------+---------+--------+--------+
    |      Name       |    SLICE   |    LUT   |     FF    |   BRAM   |   URAM  |   DSP  |   SRL  |
    +-----------------+------------+----------+-----------+----------+---------+--------+--------+
    |     PWM_GEN     |      0     |   1128   |    1387   |     0    |     0   |    6   |    0   |
    +-----------------+------------+----------+-----------+----------+---------+--------+--------+


.. toctree::
    :maxdepth: 1
