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

.. _l1_manual_ip_svpwm_duty:

============================================
Space Vector Pulse Width Modulation on Duty
============================================

Space Vector Pulse Width Modulation on Duty (IP_SVPWM_DUTY) control example resides in ``L1/tests/IP_SVPWM_DUTY`` directory. The tutorial provides a step-by-step guide that covers commands for building and running IP.

Executable Usage
==================

* **Work Directory(Step 1)**

The steps for library download and environment setup can be found in :ref:`l1_vitis_motorcontrol`. To get the design,

.. code-block:: bash

   cd L1/tests/IP_SVPWM_DUTY

* **Run and Build IP(Step 2)**

Run the following make command to build your IP targeting a specific device. This process might take long.

.. code-block:: bash

   make run CSIM=1 CSYNTH=1 COSIM=1 XPART=xc7z010-clg400-1
   
.. code-block:: bash

   make run VIVADO_SYN=1 VIVADO_IMPL=1 XPART=xc7z010-clg400-1

Note: Default arguments are set in run_hls.tcl

* **Example output(Step 2)** 

.. code-block:: bash

   SIM_SVPWM: --------------------------------------------------------------------------------------------------------------------------------------------------------
   SIM_SVPWM: Item:  | Vcmd_a	 High% Low% Dead%  High   Low  Dead  | Vcmd_b	 High% Low% Dead%  High   Low  Dead  | Vcmd_c	 High% Low% Dead%  High   Low  Dead
   SIM_SVPWM: --------------------------------------------------------------------------------------------------------------------------------------------------------
   SIM_SVPWM:    0   | 0.0 (V)	  49%   49%    0%  2499  2491    10  | 0.0 (V)	  49%   49%    0%  2499  2491    10  | 0.0 (V)	  49%   49%    0%  2499  2491    10
   SIM_SVPWM:    1   | 7.4 (V)	  74%   24%    0%  3742  1248    10  | -14.1(V)	  25%   74%    0%  1257  3733    10  | 6.6 (V)	  72%   26%    0%  3644  1346    10
   SIM_SVPWM:    2   | 14.0(V)	  74%   24%    0%  3747  1243    10  | -7.6(V)	  25%   74%    0%  1252  3738    10  | -6.5(V)	  27%   72%    0%  1382  3608    10
   SIM_SVPWM:    3   | 8.2 (V)	  75%   24%    0%  3785  1205    10  | 5.6 (V)	  69%   30%    0%  3481  1509    10  | -14.0(V)	  24%   75%    0%  1214  3776    10
   SIM_SVPWM:    4   | -8.0(V)	  24%   75%    0%  1230  3760    10  | 14.0(V)	  75%   24%    0%  3769  1221    10  | -6.1(V)	  28%   70%    0%  1447  3543    10
   SIM_SVPWM:    5   | -14.1(V)	  25%   74%    0%  1268  3722    10  | 6.7 (V)	  73%   26%    0%  3666  1324    10  | 7.2 (V)	  74%   25%    0%  3731  1259    10
   SIM_SVPWM:    6   | -7.3(V)	  25%   74%    0%  1268  3722    10  | -6.8(V)	  26%   73%    0%  1333  3657    10  | 14.0(V)	  74%   25%    0%  3731  1259    10
   SIM_SVPWM:    7   | 6.2 (V)	  71%   28%    0%  3579  1411    10  | -14.1(V)	  24%   75%    0%  1235  3755    10  | 7.8 (V)	  75%   24%    0%  3764  1226    10
   SIM_SVPWM:    8   | 24.0(V)	  49%   49%    0%  2499  2491    10  | 24.0(V)	  49%   49%    0%  2499  2491    10  | 24.0(V)	  49%   49%    0%  2499  2491    10
   SIM_SVPWM:    9   | -24.0(V)	  49%   49%    0%  2499  2491    10  | -24.0(V)	  49%   49%    0%  2499  2491    10  | -24.0(V)	  49%   49%    0%  2499  2491    10


   SIM_SVPWM: **************************************     Global Const Parameters    ****************************************************
   SIM_SVPWM: ** NAME              Type    	Hex Value        Physic Value   Unit           ValueFormat      Command-line   
   SIM_SVPWM: ** TESTNUMBER        const   	0x       a	           10 	               	  long
   SIM_SVPWM: ** clock_freq.       const   	0x 5f5e100	          100 	MHz            	   int
   SIM_SVPWM: **************************************************************************************************************************


   SIM_SVPWM: **************************************************************************************************************************
   SIM_SVPWM: --------------------------------------------    SVPWM_DUTY SECTION   -----------------------------------------------------
   SIM_SVPWM: ******************************************      AXI-lite Parameter    ****************************************************
   SIM_SVPWM: ** NAME              Type    	Hex Value        Physic Value   Unit           ValueFormat      Command-line   
   SIM_SVPWM: ** stt_cnt_iter      Read    	0x       a	           10 	times          	   int
   SIM_SVPWM: ** args_dc_link_ref  Write   	0x  180000	     24.00000	 V         	   q15q16 
   SIM_SVPWM: ** args_dc_src_mode  Write   	0x       0	   DC_SRC_ADC					[-dc_adc/-dc_ref]
   SIM_SVPWM: ** args_sample_ii    Write   	0x    1388	         5000 	               	   int		[-ii <sampling II>]
   SIM_SVPWM: ** args_sample_ii    IMPORTANT NOTICE for value setting:
   SIM_SVPWM: **                   for CSIM  	 = clock_freq / args_pwm_freq
   SIM_SVPWM: **                   for COSIM 	 depends on the latency of cascading consumer.
   SIM_SVPWM: **                   for HW run	 '1' is better to avoid backpressure to upstream ADC 
   SIM_SVPWM: --------------------------------------------------------------------------------------------------------------------------
   SIM_SVPWM: ---------------------------------------------   Static data types --------------------------------------------------------
   SIM_SVPWM: **  Using ap_ufixed<24, 8> type  for Va, Vb, Vc and dc_link_adc	 using ap_ufixed<16, 0> for ratio
   SIM_SVPWM: --------------------------------------------------------------------------------------------------------------------------
   SIM_SVPWM: -------------------------------------------- Inside kernel key values ----------------------------------------------------
   SIM_SVPWM: ** dc_link_adc      stream   	0x  159999	     21.59999	 V         	  ap_ufixed<24, 8>
   SIM_SVPWM: ** V_ref            internal 	0x       0	        22	 V         	  int
   SIM_SVPWM: ----------------------------------------------    SVPWM_DUTY END   ---------------------------------------------------------
   SIM_SVPWM: **************************************************************************************************************************
   SIM_SVPWM: All 10 commands' waveform data can be found in file wave_all10
   SIM_SVPWM: csim.exe [-shift_0/-shift_120] | [-dc_adc/-dc_ref] | [-pwm_fq <pwm frequency>] | [-dead <dead cycles>] [-ii <sampling II>]
   SIM_SVPWM:          [-v0/-v1/-v2] #for selecting different test vector 

   INFO [HLS SIM]: The maximum depth reached by any hls::stream() instance in the design is 50000
   INFO: [SIM 1] CSim done with 0 errors.
   INFO: [SIM 3] *************** CSIM finish ***************


   
Profiling
=========

The hardware resource utilizations are listed in the following table.
Different tool versions might result in slightly different resources.


.. table:: Table 1 IP resources for SVPWM_DUTY 
    :align: center

    +--------------+----------+----------+----------+----------+----------+-----------------+
    |      IP      |   BRAM   |   URAM   |    DSP   |    FF    |    LUT   | Frequency(MHz)  |
    +--------------+----------+----------+----------+----------+----------+-----------------+
    |  SVPWM_DUTY  |     0    |     0    |     3    |   2346   |   1886   |       100       |
    +--------------+----------+----------+----------+----------+----------+-----------------+

Table 2 : SVPWM_DUTY control IP profiling

.. image:: /images/API_svpwm_duty.png
   :alt: SVPWM_DUTY streaming flow
   :width: 70%
   :align: center

.. note::
    | 1. Time unit: ms.   

.. toctree::
   :maxdepth: 1

