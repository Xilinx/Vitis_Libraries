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

.. _l1_manual_ip_qei:

=============================
Quadrature Encoder Interface
=============================

Quadrature Encoder Interface(QEI) control example resides in ``L1/tests/IP_QEI`` directory. The tutorial provides a step-by-step guide that covers commands for building and running IP.

Executable Usage
==================

* **Work Directory(Step 1)**

The steps for library download and environment setup can be found in :ref:`l1_vitis_motorcontrol`. For getting the design,

.. code-block:: bash

   cd L1/tests/IP_QEI

* **Run and Build IP(Step 2)**

Run the following make command to build your IP targeting a specific device. This process might take long.

.. code-block:: bash

   make run CSIM=1 CSYNTH=1 COSIM=1 XPART=xc7z010-clg400-1
   
.. code-block:: bash

   make run VIVADO_SYN=1 VIVADO_IMPL=1 XPART=xc7z010-clg400-1

Note: Default arguments are set in run_hls.tcl

* **Example output(Step 2)** 

.. code-block:: bash

   INFO: xsimkernel Simulation Memory Usage: 856640 KB (Peak: 856640 KB), Simulation CPU Usage: 410410 ms
   INFO: [Common 17-206] Exiting xsim at Tue Mar 21 20:15:45 2023...
   INFO: [COSIM 212-316] Starting C post checking ...
   SIM_QEI: *****************************  Generating Input ********************************************************
   SIM_QEI: CLK: 100M  CPR: 1000  dir: 1  rmp:  3000  angle_start: 355.0	  run( 166): 60.0 	freq_AB: 50.0k	 num_write:  332000   time_used: 0.003320(sec)   rpm_est:3012
   SIM_QEI: CLK: 100M  CPR: 1000  dir: 0  rmp:  1000  angle_start: 54.7	  run(  27): 10.0 	freq_AB: 16.7k	 num_write:  161892   time_used: 0.001619(sec)   rpm_est:1029
   SIM_QEI: CLK: 100M  CPR: 1000  dir: 0  rmp:  2000  angle_start: 45.0	  run(  55): 20.0 	freq_AB: 33.3k	 num_write:  164780   time_used: 0.001648(sec)   rpm_est:2023
   SIM_QEI: CLK: 100M  CPR: 1000  dir: 0  rmp:  5000  angle_start: 25.2	  run(  55): 20.0 	freq_AB: 83.3k	 num_write:   66000   time_used: 0.000660(sec)   rpm_est:5051
   SIM_QEI: CLK: 100M  CPR: 1000  dir: 1  rmp:   100  angle_start: 5.4	  run(   5): 2.0 	freq_AB: 1.7k	 num_write:  299980   time_used: 0.003000(sec)   rpm_est:111
   SIM_QEI: CLK: 100M  CPR: 1000  dir: 1  rmp:   300  angle_start: 7.2	  run(   8): 3.0 	freq_AB: 5.0k	 num_write:  160000   time_used: 0.001600(sec)   rpm_est:313
   SIM_QEI: CLK: 100M  CPR: 1000  dir: 1  rmp:   500  angle_start: 9.7	  run(  13): 5.0 	freq_AB: 8.3k	 num_write:  155948   time_used: 0.001559(sec)   rpm_est:534
   QEI_GEN total writing =1343600 < 10000000
   SIM_QEI: *****************************  Output analysis  ********************************************************
   SIM_QEI: ** Changed out:    1  dir: 1  rpm:  3000  angle_detected: 0.0	 counter:    0	 err = 0
   SIM_QEI: ** Changed out:  665  dir: 0  rpm:  1000  angle_detected: 55.1	 counter:  153	 err = 0
   SIM_QEI: ** Changed out:  773  dir: 0  rpm:  2002  angle_detected: 45.4	 counter:  126	 err = 0
   SIM_QEI: ** Changed out:  993  dir: 0  rpm:  5000  angle_detected: 25.6	 counter:   71	 err = 0
   SIM_QEI: ** Changed out: 1213  dir: 1  rpm:   100  angle_detected: 5.8	 counter:   16	 err = 0
   SIM_QEI: ** Changed out: 1233  dir: 1  rpm:   300  angle_detected: 7.6	 counter:   21	 err = 0
   SIM_QEI: ** Changed out: 1265  dir: 1  rpm:   500  angle_detected: 10.4	 counter:   29	 err = 0
   SIM_QEI: *****************************AXI Parameter Value********************************************************
   SIM_QEI: ** NAME              Type    Hex Value        Physic Value                                              
   SIM_QEI: --------------------------------------------------------------------------------------------------------
   SIM_QEI: ** args_cpr          Write   0x     3e8	   1000 
   SIM_QEI: ** args_ctrl         Write   0x       0	 B_Leading_A
   SIM_QEI: ** stts_RPM_THETA_m  Read    0x  2901f4
   SIM_QEI:      |-RPM  (15, 0)  Read    0x     1f4	    500
   SIM_QEI:      |-THETA(31, 16) Read    0x      29	     41 / cpr * 2 * PI = 0.2576(Rad)
   SIM_QEI: ** stts_dir          Read    0x       1	 Clockwise
   SIM_QEI: ** stts_err          Read    0x       0	 
   SIM_QEI: ** args_cnt_trip     Write   0x  989680	  10000000 cycles = 0.100000 sec
   SIM_QEI: *********************************************************************************************************
   SIM_QEI: All waveform data can be found in file qei.log

   INFO [HLS SIM]: The maximum depth reached by any hls::stream() instance in the design is 1343600
   INFO: [COSIM 212-1000] *** C/RTL co-simulation finished: PASS ***
   
Profiling
=========

The hardware resource utilizations are listed in the following table.
Different tool versions might result in slightly different resources.


.. table:: Table 1 IP resources for quadrature encoder interface 
    :align: center

    +------------+----------+----------+----------+----------+---------+-----------------+
    |     IP     |   BRAM   |   URAM   |    DSP   |    FF    |   LUT   | Frequency(MHz)  |
    +------------+----------+----------+----------+----------+---------+-----------------+
    |    QEI     |     0    |     0    |     5    |    948   |    904  |       100       |
    +------------+----------+----------+----------+----------+---------+-----------------+

Table 2 : Quadrature Encoder Interface control IP profiling

.. image:: /images/qei_profiling.png
   :alt: Sensor based field-orientated control
   :width: 70%
   :align: center

.. note::
    | 1. Time unit: ms.   

.. toctree::
   :maxdepth: 1

