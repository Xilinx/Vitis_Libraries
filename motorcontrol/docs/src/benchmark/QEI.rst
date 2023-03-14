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

.. _l1_manual_ip_qei:

=============================
Quadrature Encoder Interface
=============================

Quadrature Encoder Interface(QEI) control example resides in ``L1/tests/IP_QEI`` directory. The tutorial provides a step-by-step guide that covers commands for building and running IP.

Executable Usage
==================

* **Work Directory(Step 1)**

The steps for library download and environment setup can be found in :ref:`l2_vitis_motorcontrol`. For getting the design,

.. code-block:: bash

   cd L1/tests/IP_QEI

* **Run and Build IP(Step 2)**

Run the following make command to build your IP targeting a specific device. Please be noticed that this process will take a long time, maybe couple of hours.

.. code-block:: bash

   make run CSIM=1 CSYNTH=1 COSIM=1 XPART=xc7z010-clg400-1
   
.. code-block:: bash

   make run VIVADO_SYN=1 VIVADO_IMPL=1 XPART=xc7z010-clg400-1

Note: Default arguments are set in run_hls.tcl

* **Example output(Step 2)** 

.. code-block:: bash

   SIM_QEI: *****************************  Generating Input ********************************************************
   INFO: [Common 17-206] Exiting xsim at Mon Jan 16 21:57:07 2023...
   INFO: [COSIM 212-316] Starting C post checking ...
   SIM_QEI: *****************************  Generating Input ********************************************************
                                              ...
   QEI_GEN total writing =1343600 < 10000000
   SIM_QEI: *****************************  Output analysis  ********************************************************
   SIM_QEI: ** Changed out:    1  dir: 1  rpm:  3000  angle_detected: 0.0     counter:    0   err = 0
   SIM_QEI: ** Changed out:  665  dir: 0  rpm:  1000  angle_detected: 55.1    counter:  153   err = 0
   SIM_QEI: ** Changed out:  773  dir: 0  rpm:  2002  angle_detected: 45.4    counter:  126   err = 0
   SIM_QEI: ** Changed out:  993  dir: 0  rpm:  5000  angle_detected: 25.6    counter:   71   err = 0
   SIM_QEI: ** Changed out: 1213  dir: 1  rpm:   100  angle_detected: 5.8     counter:   16   err = 0
   SIM_QEI: ** Changed out: 1233  dir: 1  rpm:   300  angle_detected: 7.6     counter:   21   err = 0
   SIM_QEI: ** Changed out: 1265  dir: 1  rpm:   500  angle_detected: 10.4    counter:   29   err = 0
   SIM_QEI: *****************************AXI Parameter Value********************************************************
   SIM_QEI: ** NAME              Type    Hex Value        Physic Value
   SIM_QEI: --------------------------------------------------------------------------------------------------------
   SIM_QEI: ** args_cpr          Write   0x     3e8     1000
   SIM_QEI: ** args_ctrl         Write   0x       0   A_Leading_B
   SIM_QEI: ** stts_RPM_THETA_m  Read    0x  2901f4
   SIM_QEI:      |-RPM  (15, 0)  Read    0x     1f4      500
   SIM_QEI:      |-THETA(31, 16) Read    0x      29       41 / cpr * 2 * PI = 0.2576(Rad)
   SIM_QEI: ** stts_dir          Read    0x       1   Clockwise
   SIM_QEI: ** stts_err          Read    0x       0
   SIM_QEI: ** args_cnt_trip     Write   0x  989680    10000000 cycles = 0.100000 sec
   SIM_QEI: *********************************************************************************************************
   SIM_QEI: All waveform data can be found in file qei.log

   INFO [HLS SIM]: The maximum depth reached by any hls::stream() instance in the design is 1343600
   INFO: [COSIM 212-1000] *** C/RTL co-simulation finished: PASS ***
   
Profiling
=========

The hardware resource utilizations are listed in the following table.
Different tool versions may result slightly different resource.


.. table:: Table 1 IP resources for quadrature encoder interface 
    :align: center

    +------------+----------+----------+----------+----------+---------+-----------------+
    |     IP     |   BRAM   |   URAM   |    DSP   |    FF    |   LUT   | Frequency(MHz)  |
    +------------+----------+----------+----------+----------+---------+-----------------+
    |    QEI     |     0    |     0    |     5    |    976   |    867  |       300       |
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

