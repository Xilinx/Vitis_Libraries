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


******************************
Internal Design of SVPWM_DUTY
******************************

Overview
===========

This API is a submodule of SVPWM function. A complete SVPWM function is composed of two parts: SVPWM_DUTY and PWM_GEN. This API is the SVPWM_DUTY. It is a fully optimized implementation through the Xilinx HLS design methodology. It can produce the duty cycles of the three-phase currents. It has configurable parameters via AXI-Lite entries, include phase_shift, dclink_src, pwm_freq and dead_cycles, etc. The outputs are the duty cycles of each phase during every pwm cycle.

Implemention
============

The detail algorithm implemention is illustrated as below:

.. image:: /images/API_svpwm_duty.png
   :alt: design of duty cycle
   :width: 60%
   :align: center

As it is shown in the aboved pictures, the entire SVPWM_DUTY have six configurable parameters.

* phase_shift: flag "[-shift_0/-shift_120]". It determines whether the input voltage streams have phase shift. 

* dclink_source: flag "[-dc_adc/-dc_ref]". This flag register determines the dclink voltage source.

* pwm_freq: flag "[-pwm_fq <pwm frequency>]". This flag register provides a configurable entry for the pwm wave frequency. The pwm wave frequency is also the throughput at the output end.

* dead_cycles: flag "[-dead <dead cycles>]". The dead_cycles determines the transit time between the switch on/off. The switches pair shall not simultaneously be switching on and off, in terms of the danger of overloaded transient currents on the bridge. It may incur the systematic turbulence and cause serious problem. The default value of dead_cycles is 10.  

* sampling ii: flag "[-ii <sampling II>]". The ii (iteration interval) determines the sampling rate of the input, which is supposed to be slower than calculate_ratios.


Profiling 
============

The Post-Synthesis Resource usage are shown in the table1 below.
The SVPWM_DUTY C/RTL co-simulation on CPU, and the result is based it in table2.   

.. table:: Table 1 Post-Synthesis Resource usage
    :align: center

    +------------------+------------+----------+-----------+----------+---------+--------+--------+
    |       Name       |    SLICE   |    LUT   |     FF    |   BRAM   |   URAM  |   DSP  |   SRL  |
    +------------------+------------+----------+-----------+----------+---------+--------+--------+
    |    SVPWM_DUTY    |      0     |   2116   |    2358   |     0    |     0   |    7   |    6   |
    +------------------+------------+----------+-----------+----------+---------+--------+--------+


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
