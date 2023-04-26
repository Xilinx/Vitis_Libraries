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

.. _l1_manual_ip_pwm_gen:

================================
Pulse Width Modulation Generator
================================

Pulse Width Modulation Generator (PWM_GEN) control example resides in ``L1/tests/IP_PWM_GEN`` directory. The tutorial provides a step-by-step guide that covers commands for building and running IP.

Executable Usage
===================

* **Work Directory(Step 1)**

The steps for library download and environment setup can be found in :ref:`l1_vitis_motorcontrol`. For getting the design,

.. code-block:: bash

   cd L1/tests/IP_PWM_GEN

* **Run and Build IP(Step 2)**

Run the following make command to build your IP targeting a specific device. Please be noticed that this process will take a long time, maybe couple of hours.

.. code-block:: bash

   make run CSIM=1 CSYNTH=1 COSIM=1 XPART=xc7z010-clg400-1
   
.. code-block:: bash

   make run VIVADO_SYN=1 VIVADO_IMPL=1 XPART=xc7z010-clg400-1

Note: Default arguments are set in run_hls.tcl

* **Example output(Step 2)** 

.. code-block:: bash

   SIM_SVPWM: --------------------------------------------------------------------------------------------------------------------------------------------------------
   SIM_SVPWM: Item:  | Vcmd_a       High% Low% Dead%  High   Low  Dead  | Vcmd_b    High% Low% Dead%  High   Low  Dead  | Vcmd_c    High% Low% Dead%  High   Low  Dead
   SIM_SVPWM: --------------------------------------------------------------------------------------------------------------------------------------------------------
   SIM_SVPWM:    0   | 0.0 (V)       54%   44%    1%   549   441    10  | 0.0 (V)    54%   44%    1%   549   441    10  | 0.0 (V)    54%   44%    1%   549   441    10
   SIM_SVPWM:    1   | 7.4 (V)       82%   16%    1%   823   167    10  | -14.1(V)   27%   71%    1%   276   714    10  | 6.6 (V)    80%   18%    1%   801   189    10
   SIM_SVPWM:    2   | 14.0(V)       82%   16%    1%   824   166    10  | -7.6(V)    27%   71%    1%   275   715    10  | -6.5(V)    30%   68%    1%   304   686    10
   SIM_SVPWM:    3   | 8.2 (V)       83%   15%    1%   832   158    10  | 5.6 (V)    76%   22%    1%   766   224    10  | -14.0(V)   26%   72%    1%   267   723    10
   SIM_SVPWM:    4   | -8.0(V)       27%   72%    1%   270   720    10  | 14.0(V)    82%   16%    1%   829   161    10  | -6.1(V)    31%   67%    1%   318   672    10
   SIM_SVPWM:    5   | -14.1(V)      27%   71%    1%   279   711    10  | 6.7 (V)    80%   18%    1%   806   184    10  | 7.2 (V)    82%   17%    1%   820   170    10
   SIM_SVPWM:    6   | -7.3(V)       27%   71%    1%   279   711    10  | -6.8(V)    29%   69%    1%   293   697    10  | 14.0(V)    82%   17%    1%   820   170    10
   SIM_SVPWM:    7   | 6.2 (V)       78%   20%    1%   787   203    10  | -14.1(V)   27%   71%    1%   271   719    10  | 7.8 (V)    82%   16%    1%   828   162    10
   SIM_SVPWM:    8   | 24.0(V)       54%   44%    1%   549   441    10  | 24.0(V)    54%   44%    1%   549   441    10  | 24.0(V)    54%   44%    1%   549   441    10
   SIM_SVPWM:    9   | -24.0(V)      54%   44%    1%   549   441    10  | -24.0(V)   54%   44%    1%   549   441    10  | -24.0(V)   54%   44%    1%   549   441    10

   SIM_SVPWM: **************************************     Global Const Parameters    ****************************************************
   SIM_SVPWM: ** NAME              Type            Hex Value        Physic Value   Unit           ValueFormat      Command-line
   SIM_SVPWM: ** TESTNUMBER        const           0x       a                 10                     long
   SIM_SVPWM: ** clock_freq.       const           0x 5f5e100                100   MHz                int
   SIM_SVPWM: **************************************************************************************************************************

   SIM_SVPWM: **************************************************************************************************************************
   SIM_SVPWM: ---------------------------------------------    PWM_GEN SECTION   ---------------------------------------------------------
   SIM_SVPWM: ******************************************      AXI-lite Parameter    ******************************************************
   SIM_SVPWM: ** stt_pwm_cycle     Read            0x     3e8               1000   cycle of clk_fq    int
   SIM_SVPWM: ** args_pwm_freq     Write           0x   186a0             100000   Hz                 int          [-pwm_fq <pwm frequency>]
   SIM_SVPWM: ** args_dead_cycles  Write           0x       a                 10   cycle of clk_fq    int          [-dead <dead cycles>]
   SIM_SVPWM: ** args_phase_shift  Write           0x       0         SHIFT_ZERO                                   [-shift_0/-shift_120]
   SIM_SVPWM: ** args_sample_ii    Write           0x     3e8               1000                      int          [-ii <sampling II>]
   SIM_SVPWM: ** args_sample_ii    IMPORTANT NOTICE for value setting:
   SIM_SVPWM: **                   for CSIM         = 1
   SIM_SVPWM: **                   for COSIM        default value is 1. Suggested value in run_hls_sim.tcl
   SIM_SVPWM: **                   for HW run       '1' is better to avoid backpressure to upstream ADC
   SIM_SVPWM: ---------------------------------------------      PWM_GEN END   ---------------------------------------------------------
   SIM_SVPWM: **************************************************************************************************************************
   SIM_SVPWM: All 10 commands' waveform data can be found in file wave_all10
   SIM_SVPWM: csim.exe [-shift_0/-shift_120] | [-dc_adc/-dc_ref] | [-pwm_fq <pwm frequency>] | [-dead <dead cycles>] [-ii <sampling II>]
   SIM_SVPWM:          [-v0/-v1/-v2] #for selecting different test vector


   INFO [HLS SIM]: The maximum depth reached by any hls::stream() instance in the design is 10000
   INFO: [COSIM 212-333] Generating C post check test bench ...
   INFO: [COSIM 212-12] Generating RTL test bench ...
   INFO: [COSIM 212-1] *** C/RTL co-simulation file generation completed. ***
   
Profiling
=========

The hardware resource utilizations are listed in the following table.
Different tool versions may result slightly different resource.


.. table:: Table 1 IP resources for Pulse Width Modulation Generator
    :align: center

    +------------+----------+----------+----------+-----------+----------+-----------------+
    |     IP     |   BRAM   |   URAM   |    DSP   |     FF    |    LUT   | Frequency(MHz)  |
    +------------+----------+----------+----------+-----------+----------+-----------------+
    |  PWM_GEN   |     0    |     0    |     6    |    1413   |   1094   |       100       |
    +------------+----------+----------+----------+-----------+----------+-----------------+

Table 2 : Pulse Width Modulation Generator Control IP Profiling

.. image:: /images/API_pwm_gen.png
   :alt: PWM Generator streaming flow
   :width: 70%
   :align: center

.. note::
    | 1. Time unit: ms.   

.. toctree::
   :maxdepth: 1

