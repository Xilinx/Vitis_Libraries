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


***************************
Internal Design of PWM_GEN
***************************

Overview
===========

This API is a submodule of SVPWM function. A complete SVPWM function is composed of two parts: SVPWM_DUTY and PWM_GEN. This API is the PWM_GEN. It is a fully optimized implementation through the Xilinx HLS design methodology. It can produce the bitstreams to control the switches on/off on every branch of the bridge.  The input are the duty cycles transmitted from the upstream SVPWM_DUTY. The outputs are the center-aligned bitstreams where the holding(high) time ratio in every pwm cycle are strictly complied with the input.

Implemention
============

The detail algorithm implemention is illustrated as below:

.. image:: /images/API_pwm_gen.png
   :alt: design of pwm_gen
   :width: 60%
   :align: center

As it is shown in the aboved pictures, the entire PWM_GEN have 4 functions and dataflow between these functions.

* phase_shift: flag "[-shift_0/-shift_120]". It determines whether the input voltage streams have phase shift.  

* pwm_freq: flag "[-pwm_fq <pwm frequency>]". This flag register provides a configurable entry for the pwm wave frequency. The pwm wave frequency is also the throughput at the output end.

* dead_cycles: flag "[-dead <dead cycles>]". The dead_cycles determines the transit time between the switch on/off. The switches pair shall not simultaneously be switching on and off, in terms of the danger of overloaded transient currents on the bridge. It may incur the systematic turbulence and cause serious problem. The default value of dead_cycles is 10. 

* stt_pwm_cycle: this parameter is to monitor the status of pwm_cycle length, which is supposed to be clk_freq/pwm_freq.

* sampling ii: flag "[-ii <sampling II>]". The hls_pwm_gen's sampling rate should be strictly set as 500, since the sampler's ii is equal to 2. It makes hls_pwm_gen's top consumes AXIS every 1000 cycles, which matchs the hls_svpwm_duty.

Profiling 
============

The Post-Synthesis Resource usage are shown in the table1 below.
The PWM_GEN C/RTL co-simulation on CPU, and the result is based it in table2.   

.. table:: Table 1 Post-Synthesis Resource usage
    :align: center

    +-----------------+------------+----------+-----------+----------+---------+--------+--------+
    |      Name       |    SLICE   |    LUT   |     FF    |   BRAM   |   URAM  |   DSP  |   SRL  |
    +-----------------+------------+----------+-----------+----------+---------+--------+--------+
    |     PWM_GEN     |      0     |   1239   |    1673   |     0    |     0   |    6   |   40   |
    +-----------------+------------+----------+-----------+----------+---------+--------+--------+


.. .. table:: Table 2 Comparison between svpwm_duty on CPU and Cosim
..     :align: center
    
..     +------------------+----------+-----------+------------+------------+
..     |      Image       |   Size   |    Cpu    |   Cosim    |   Speed    |
..     +------------------+----------+-----------+------------+------------+
..     |      t0.png      |   960    |   0.139   |   0.058    |    2.4x    |
..     +------------------+----------+-----------+------------+------------+
..     | Ali_512x512.png  |   960    |   0.233   |   0.058    |    4.0x    |
..     +------------------+----------+-----------+------------+------------+
..     |   853x640.png    |   1152   |   0.152   |   0.069    |    2.2x    |
..     +------------------+----------+-----------+------------+------------+
..     |   hq_2Kx2K.png   |   1152   |   0.120   |   0.069    |    1.7x    |
..     +------------------+----------+-----------+------------+------------+

.. .. note::
..     | 1. orderTokenize running on platform with Intel(R) Xeon(R) CPU E5-2690 v4 @ 2.60GHz, 28 Threads (14 Core(s)).
..     | 2. Time unit: ms.

.. toctree::
    :maxdepth: 1
