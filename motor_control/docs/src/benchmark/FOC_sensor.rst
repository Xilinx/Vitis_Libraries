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

.. _l1_manual_ip_foc_senser:

=============================================
Sensor based field-orientated control (FOC)
=============================================

Sensor based field-orientated control(FOC) example resides in ``L1/tests/IP_FOC`` directory. The tutorial provides a step-by-step guide that covers commands for building and running IP.

Executable Usage
==================

* **Work Directory(Step 1)**

The steps for library download and environment setup can be found in :ref:`l1_vitis_motorcontrol`. To get the design,

.. code-block:: bash

   cd L1/tests/IP_FOC

* **Run and Build IP(Step 2)**

Run the following make command to build your IP targeting a specific device. This process takes long.

.. code-block:: bash

   make run CSIM=1 CSYNTH=1 COSIM=1 XPART=xc7z010-clg400-1

The command creates IP*sim.project for simulation.

.. code-block:: bash

   make run VIVADO_SYN=1 VIVADO_IMPL=1 XPART=xc7z010-clg400-1

The command creates IP.project for export IP.

.. Note:: Default arguments are set in run_hls.tcl.

* **Example output(Step 2)** 

.. code-block:: bash

   SIM_FOC_M:********** Simulation parameters ************* Motor parameter ******************** Log files***************************************
   SIM_FOC_M:  Timescale  :     10 (us)    |  motor.w     : 718.0615       (rad/s) |  Log of parameters : sim_torqueWithoutSpeed.para.foc
   SIM_FOC_M:  Total step :   3000         |  motor.theta :  5.014019      (rad)   |  Log of FOC inputs : sim_torqueWithoutSpeed.in.foc
   SIM_FOC_M:  Total time : 0.030000 (s)   |  motor.Id    : 0.334735       ( A )   |  Log of FOC outputs: sim_torqueWithoutSpeed.out.foc
   SIM_FOC_M:  Inteval    :      3         |  motor.Iq    : 1.253668       ( A )
   SIM_FOC_M:  FOC MODE   : MOD_TORQUE_WITHOUT_SPEED
   SIM_FOC_M:  FOC CPR    :   1000         |  FOC PPR:      2
   SIM_FOC_M:************ PID Final Status *********
   SIM_FOC_M:  SPEED SP   : 10000.0        |  FLUX SP: 0.0000                      |  TORQUE SP: 4.8000            |  FW SP: --
   SIM_FOC_M:  SPEED KP   : 2.7000         |  FLUX KP: 1.0000                      |  TORQUE KP: 5.0000            |  FW KP: --
   SIM_FOC_M:  SPEED KI   : 0.0033         |  FLUX KI: 0.0000                      |  TORQUE KI: 0.0033            |  FW KI: --
   SIM_FOC_M:  SPEED ERR  : 3144.000       |  FLUX ERR:  -0.332                    |  TORQUE ERR:  3.5469  |  FW ERR: --
   SIM_FOC_M:  SPEED ACC  : 23853.000      |  FLUX ACC: -862.297                   |  TORQUE ACC: 9952.8320        |  FW ACC: --
   SIM_FOC_M:************************************************************************************************************************************
   ...


.. Note:: The current test is a hybrid test of the eight modes run serially. Each mode's simulation parameter and Motor parameter is shown up. For example: 

   * The title of "simulation parameters" shows the setting for simulation. The first 3000 test steps use MOD_SPEED_WITH_TORQUE, Speed setpoint is 10000, as we could check the "motor parameter" motor.w=1030.652 (rad/s) is near the setting rpm 10000. (RPM = motor.w * 60 / (2 * pi) ).

   * The title of "log files" shows the log files generate for this 3000 steps. They are used as an input and golden files for the file flow test, which better simulate the actual running. Then still apply MOD_SPEED_WITH_TORQUE for the next 3000 steps, Speed setpoint is -16000. This runs to another direction.

Now run the Model Based Sim to get the input and output of FOC with Motor module, and save them to files. This sim restarts the FOC ip for every input.

Then, run the File Based Sim to simulate the actually status when FOC IP is running. Benchmark system is shown in the following figure:

Figure 1 : Benchmark system of field-orientated control(FOC)

.. image:: /images/foc_test.png
   :alt: Benchmark system of FOC
   :width: 70%
   :align: center

Motor speed in 8 modes in simulation is shown by Figure 2:

Figure 2 : Motor speed in 8 modes in simulation

.. image:: /images/foc_8_modes.png
   :alt: Motor speed in 8 modes in simulation
   :width: 70%
   :align: center

The Motor Model parameters by default simulation and Derived Motor Configuration is setting in the commen.hpp, and are shown in the following tables:

Table 1 : Motor Model parameters by default

.. image:: /images/Motor_parameters.png
   :alt: Motor parameters
   :width: 40%
   :align: center

Table 2 : Derived Motor Configuration

.. image:: /images/Derived_Motor_Configuration.png
   :alt: Derived Motor Configuration
   :width: 40%
   :align: center

* Important: Change the sine and cosine tables in the file foc.h accordingly when changing this CPR(COMM_MACRO_CPR) in the commen.hpp .

Static Parameter of FOC in simulation

.. image:: /images/Static_parameter_foc.png
   :alt: Static Parameter of IP
   :width: 70%
   :align: center

AXI-lite Parameter of FOC setting in simulation

.. image:: /images/AXI_LITE_parameter_FOC.png
   :alt: AXI-lite Parameter of IP
   :width: 70%
   :align: center

Other AXI-lite Parameter is all setting to zero in the first mode in simulation.

File Based Simulation of FOC IP's result is shown below.

.. code-block:: bash

   SIM_FOC_F:****Loading parameters and input from files ************************************************************************************
   SIM_FOC_F:  parameters file is sim_torqueWithoutSpeed.para.foc, format:
   SIM_FOC_F:  FOC inputs file is sim_torqueWithoutSpeed.in.foc, format: <float va> <float vb> <float vb> <theta_m, rpm> <int va> <int vb> <int vc>
   SIM_FOC_F:  FOC output file is sim_torqueWithoutSpeed.out.foc, format: <float va> <float vb> <float vb> <theta_m, rpm> <int va> <int vb> <int vc>
   SIM_FOC_F:  MODE      : 2
   SIM_FOC_F:  FLUX_SP   : 0.000000
   SIM_FOC_F:  FLUX_KP   : 1.000000
   SIM_FOC_F:  FLUX_KI   : 0.000000
   SIM_FOC_F:  TORQUE_SP : 4.799988
   SIM_FOC_F:  TORQUE_KP : 5.000000
   SIM_FOC_F:  TORQUE_KI : 0.003311
   SIM_FOC_F:  SPEED_SP  : 10000.000000
   SIM_FOC_F:  SPEED_KP  : 2.699997
   SIM_FOC_F:  SPEED_KI  : 0.003311
   SIM_FOC_F:  VD        : 0.000000
   SIM_FOC_F:  VQ        : 15000.000000
   SIM_FOC_F:  FW_KP     : 1.000000
   SIM_FOC_F:  FW_KI     : 0.003311
   SIM_FOC_F:  CNT_TRIP  : 3000
   SIM_FOC_F: ***********   Comparison with golden file: ********
   SIM_FOC_F:  Total step  :   3000
   SIM_FOC_F:  Golden File : sim_torqueWithoutSpeed.out.foc
   SIM_FOC_F:  Max Voltage : 24.000        100.00%
   SIM_FOC_F:  threshold   : 1.200 5.00%
   SIM_FOC_F:  Mean error  : 0.000 0.00%
   SIM_FOC_F:  Max error   : 0.000 0.00%
   SIM_FOC_F:  Min error   : 0.000 0.00%
   SIM_FOC_F:  Total error :      0
   SIM_FOC_F:********************************************************************************************************************************
   ...

MOD_MANUAL_TORQUE_FLUX_FIXED_SPEED is a MANUAL mode for an expert, at motor start to get the encoder in initial calibration and alignment prior to starting a closed loop control function.
So, simulate this mode after STOP mode. If Fixperiod_args in AXI-lite is set to 1, it is a fixed step size in every clk/(Fixperiod_args + 1). 

After the conversion,

When clk equal 100M, Fixperiod_args is 1, Theoretical RPM is 600000.

When clk equal 100M, Fixperiod_args is 3999, Theoretical RPM is 300.

(II = 5, interval cycle = (FixedPeriod+1)*II), RPM = 60/(interval cycle/freq*CPR))

.. Note:: Test summary all the result of eight modes on the file flow.

.. code-block:: bash
   

   SIM_FOC********************************************************** TEST SUMMARY ***********************************************************
   SIM_FOC_M:  ------ Summary for Model-based simulation -----------------------------------------------------------
   SIM_FOC_M:  Kernel sampling mode           : one calling one sample
   SIM_FOC_M:  Simulation resolution          : 0.010000 (ms)
   SIM_FOC_M:  Simulation total time          : 270.000000 (ms)
   SIM_FOC_M:  Inteval for printing wave data : 3
   SIM_FOC_M:  Wave data for all 9 phases test: sim_foc_ModelFoc.log
   SIM_FOC_M:  Phase-1 generated files        : sim_torqueWithoutSpeed<.para.foc> <.in.foc> <.out.foc>
   SIM_FOC_M:  Phase-2 generated files        : sim_rpm10k<.para.foc> <.in.foc> <.out.foc>
   SIM_FOC_M:  Phase-3 generated files        : sim_rpm16k<.para.foc> <.in.foc> <.out.foc>
   SIM_FOC_M:  Phase-4 generated files        : sim_rpm16k_weak<.para.foc> <.in.foc> <.out.foc>
   SIM_FOC_M:  Phase-5 generated files        : sim_manualTorqueFlux<.para.foc> <.in.foc> <.out.foc>
   SIM_FOC_M:  Phase-6 generated files        : sim_manualTorque<.para.foc> <.in.foc> <.out.foc>
   SIM_FOC_M:  Phase-7 generated files        : sim_manualFlux<.para.foc> <.in.foc> <.out.foc>
   SIM_FOC_M:  Phase-8 generated files        : sim_stop<.para.foc> <.in.foc> <.out.foc>
   SIM_FOC_M:  Phase-9 generated files        : sim_manualTorqueFluxFixedSpeed1<.para.foc> <.in.foc> <.out.foc>
   SIM_FOC_F:  ------ Summary for File-based simulation based on Model-based outputs -------------------------------
   SIM_FOC_F:  Kernel sampling mode           : one calling multi-sample
   SIM_FOC_F:  Phase-1: 0.000(ms) ~ 30.000(ms)      Mode: MOD_TORQUE_WITHOUT_SPEED           RPM: 10000     Sampling II: Depending on II after synthesis    Over threshold(1.20V): 0
   SIM_FOC_F:  Phase-2: 30.000(ms) ~ 60.000(ms)     Mode: MOD_SPEED_WITH_TORQUE              RPM: 10000     Sampling II: Depending on II after synthesis    Over threshold(1.20V): 0
   SIM_FOC_F:  Phase-3: 60.000(ms) ~ 90.000(ms)     Mode: MOD_SPEED_WITH_TORQUE              RPM: -16000    Sampling II: Depending on II after synthesis    Over threshold(1.20V): 0
   SIM_FOC_F:  Phase-4: 90.000(ms) ~ 120.000(ms)    Mode: MOD_FLUX                           RPM: -16000    Sampling II: Depending on II after synthesis    Over threshold(1.20V): 0
   SIM_FOC_F:  Phase-5: 120.000(ms) ~ 150.000(ms)   Mode: MOD_MANUAL_TORQUE_FLUX             RPM: -16000    Sampling II: Depending on II after synthesis    Over threshold(1.20V): 0
   SIM_FOC_F:  Phase-6: 150.000(ms) ~ 180.000(ms)   Mode: MOD_MANUAL_TORQUE                  RPM: -16000    Sampling II: Depending on II after synthesis    Over threshold(1.20V): 0
   SIM_FOC_F:  Phase-7: 180.000(ms) ~ 210.000(ms)   Mode: MOD_MANUAL_FLUX                    RPM: -16000    Sampling II: Depending on II after synthesis    Over threshold(1.20V): 0
   SIM_FOC_F:  Phase-8: 210.000(ms) ~ 240.000(ms)   Mode: MOD_STOPPED                        RPM: -16000    Sampling II: Depending on II after synthesis    Over threshold(1.20V): 0
   SIM_FOC_F:  Phase-9: 240.000(ms) ~ 270.000(ms)   Mode: MOD_MANUAL_TORQUE_FLUX_FIXED_SPEED RPM: -16000    Sampling II: Depending on II after synthesis    Over threshold(1.20V).
   SIM_FOC_F:********************************************************************************************************************************




   csim.exe [-dt <time scale in sec> ]      Simulation resolution
            [-pii <print interval>]         Wave data filesim_foc_ModelFoc.log sampling interval
            [-log <char*>]                  Setting log files prefix with surfix _ModelFoc.log
            [-range]                        Enable range tracing for internal varialbes in FOC
            [-cnt <Number of input>]        Number of sample for a mode

   INFO [HLS SIM]: The maximum depth reached by any hls::stream() instance in the design is 3000
   INFO: [SIM 211-1] CSim done with 0 errors.

   
Profiling
=========

The hardware resource utilizations are listed in the following table.
Different tool versions might result in slightly different resources.
The max throughput is 20M/s by 22.2 and 23.2 .

* Important: Change the sine and cosine tables in the file foc.h accordingly when changing this CPR(COMM_MACRO_CPR) in the commen.hpp .


.. table:: Table 3 IP resources for Sensor based field-orientated control in 23.2 
    :align: center

    +------------+----------+----------+----------+----------+---------+-----------------+
    |     IP     |   BRAM   |   URAM   |    DSP   |    FF    |   LUT   | Frequency(MHz)  |
    +------------+----------+----------+----------+----------+---------+-----------------+
    | FOC_sensor |     2    |     0    |     67   |   5019   |   5179  |       100       |
    +------------+----------+----------+----------+----------+---------+-----------------+

Table 4 : IP profiling of Sensor based field-orientated control

.. image:: /images/foc_8_modes.png
   :alt: Sensor based field-orientated control
   :width: 70%
   :align: center

.. note::
    | 1. Time unit: 10us per sample.   

.. toctree::
   :maxdepth: 1

