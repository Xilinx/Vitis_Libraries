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

.. meta::
   :keywords: Vitis, Motor Control, Vitis Motor Control Library, Alveo
   :description: Vitis Motor Control Library is an open-sourced Vitis library written in C++ for accelerating Motor Control applications in a variety of use cases.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _brief:

=====================================
Vitis Motor Control Library Tutorial
=====================================

Vitis Motor Control and Hardware Acceleration
----------------------------------------------

Vitis Motor Control Library is an open-sourced Vitis library written in C++ for accelerating graph applications in a variety of use cases. It now covers a level of acceleration: the module level (L1).

Based on the Motor Control library, which provides a virtual motor model, users can complete all core module verifications such as FOC, QEI, SVPWM, solely in the Vitis environment. This greatly enhances the efficiency of design iterations. This tutorial is designed to guide users on how to conduct design verification and learn how to validate design functionality at different stages of the design process.

Lab-1: Downloading the Vitis Motor Control Library and Understanding the Virtual Motor Model
---------------------------------------------------------------------------------------------

Download the Vitis Graph Library
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: shell

   #!/bin/bash
   git clone https://github.com/Xilinx/Vitis_Libraries.git
   cd Vitis_Libraries/motorcontrol
   git checkout next

Get knowledge about directory structure of Vitis Motor Control Library
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. code-block:: shell

   ├── docs------------------------document
   │   └── src
   │       ├── benchmark
   │       ├── guide_L1
   │       │   └── IPs
   │       └── images
   └── L1
      ├── include/hw--------------L1 APIs
      ├── meta
      └── tests-------------------L1 Test
         ├── Clarke_Direct
         │   └── src
         ├── Clarke_Inverse
         │   └── src
         ├── IP_FOC_sensor
         │   └── src
         ├── IP_FOC_sensorless
         │   └── src
         ├── IP_PWM_GEN
         ├── IP_QEI
         │   └── src
         ├── IP_SVPWM_DUTY
         ├── Model---------------model files include virtual motor model
         ├── Park_Direct
         │   └── src
         ├── Park_Inverse
         │   └── src
         └── PI_Control
               └── src

Get knowledge about virtual motor model
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
   A simplified physical motor model can be represented as follows:
.. image:: /images/tutorial_motor1.png
   :alt: motor1
   :scale: 100%
   :align: center

.. image:: /images/tutorial_motor2.png
   :alt: motor2
   :scale: 100%
   :align: center

The corresponding matrix representation is shown below:

.. image:: /images/tutorial_equation.png
   :alt: equation
   :scale: 100%
   :align: center

KD240 Accessory Kit Motor Model Parameters

The KD240 Accessory Kit that will be used as the reference implementation uses the BLWR111D-24V-10000 motor from Anaheim Motors.  Their datasheet outlines the following parameters:

+-----------------------------------------+----------+-----------+
|              Parameter                  |   Unit   |   Value   |
+-----------------------------------------+----------+-----------+
|         Torque Constant (Kt)            |  oz-in/A |    2.27   |
+-----------------------------------------+----------+-----------+
|          Rotor Inertia (Ir)             |oz-in-sec2| 3.3x10^-5 |
+-----------------------------------------+----------+-----------+
|            Rated Voltage                |     V    |     24    |
+-----------------------------------------+----------+-----------+
|            Rated Torque                 |  oz-in   |    2.0    |
+-----------------------------------------+----------+-----------+
|             Rated Speed                 |   RPM    |  10,000   |
+-----------------------------------------+----------+-----------+
|             Rated Power                 |    W     |     15    |
+-----------------------------------------+----------+-----------+
|            Rated Current                |    A     |    0.88   |
+-----------------------------------------+----------+-----------+
|            Poles pairs (N)              |    #     |     2     |
+-----------------------------------------+----------+-----------+
|       Permanent magnet flux linkage     |   A/Nm   |  0.008015 |
+-----------------------------------------+----------+-----------+
|             Peak Torque                 |   oz-in  |    6.0    |
+-----------------------------------------+----------+-----------+
|            No Load Speed                |   RPM    |   12,800  |
+-----------------------------------------+----------+-----------+
|        Line-to-Line Resistance          |    Ω     |    4.63   |
+-----------------------------------------+----------+-----------+
|        Line-to-Line Inductance          |    mH    |    1.69   |
+-----------------------------------------+----------+-----------+
|   Ld = 0.5 * line-to-line inductance    |    mH    |   0.845   |
+-----------------------------------------+----------+-----------+

These parameters could be set in the head file common.hpp in ./xf_motorcontrol/L1/include/hw/ folder, the virtual motor model could be found in ./xf_motorcontrol/L1/tests/Model/model_motor.hpp.

Lab-2: Simulation and verification of FOC_sensor
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

CSIM verification

flow

.. code-block:: shell

   source <Vitis_install_path>/Vitis/2023.1/settings64.sh
   git clone https://github.com/Xilinx/Vitis_Libraries.git
   cd ./motorcontrol/
   git checkout next 
   git pull
   cd L1/tests/IP_FOC_sensor/
   make run CSIM=1  XPART=xc7z020-clg400-1

stdout explanation

.. code-block:: shell

   vim ./xf_motorcontrol/L1/tests/IP_FOC_sensor/ip_foc_periodic_ap_fixed_sim.prj/sol1/csim/report/hls_foc_periodic_ap_fixed_csim.log

   SIM_FOC_M:********** Simulation parameters ************* Motor parameter ******************** Log files***************************************
   SIM_FOC_M:  Timescale  :     10 (us)    |  motor.w     : 718.0615   (rad/s) |  Log of parameters : sim_torqueWithoutSpeed.para.foc
   SIM_FOC_M:  Total step :   3000         |  motor.theta :  5.014019  (rad)   |  Log of FOC inputs : sim_torqueWithoutSpeed.in.foc
   SIM_FOC_M:  Total time : 0.030000 (s)   |  motor.Id    : 0.334735   ( A )   |  Log of FOC outputs: sim_torqueWithoutSpeed.out.foc
   SIM_FOC_M:  Inteval    :      3         |  motor.Iq    : 1.253668   ( A ) 
   SIM_FOC_M:  FOC MODE   : MOD_TORQUE_WITHOUT_SPEED          
   SIM_FOC_M:  FOC CPR    :   1000         |  FOC PPR:      2    
   SIM_FOC_M:************ PID Final Status *********
   SIM_FOC_M:  SPEED SP   : 10000.0    |  FLUX SP: 0.0000              |  TORQUE SP: 4.8000        |  FW SP: --
   SIM_FOC_M:  SPEED KP   : 2.7000     |  FLUX KP: 1.0000              |  TORQUE KP: 5.0000        |  FW KP: --
   SIM_FOC_M:  SPEED KI   : 0.0033     |  FLUX KI: 0.0000              |  TORQUE KI: 0.0033        |  FW KI: --
   SIM_FOC_M:  SPEED ERR  : 3144.000   |  FLUX ERR:  -0.332            |  TORQUE ERR:  3.5469  |  FW ERR: --
   SIM_FOC_M:  SPEED ACC  : 23853.000  |  FLUX ACC: -862.297           |  TORQUE ACC: 9952.8320    |  FW ACC: --
   SIM_FOC_M:************************************************************************************************************************************
   SIM_FOC_M:********** Simulation parameters ************* Motor parameter ******************** Log files***************************************
   SIM_FOC_M:  Timescale  :     10 (us)    |  motor.w     : 1030.6429  (rad/s) |  Log of parameters : sim_rpm10k.para.foc
   SIM_FOC_M:  Total step :   3000         |  motor.theta :  4.277364  (rad)   |  Log of FOC inputs : sim_rpm10k.in.foc
   SIM_FOC_M:  Total time : 0.030000 (s)   |  motor.Id    : 0.079221   ( A )   |  Log of FOC outputs: sim_rpm10k.out.foc
   SIM_FOC_M:  Inteval    :      3         |  motor.Iq    : -0.002429  ( A ) 
   SIM_FOC_M:  FOC MODE   : MOD_SPEED_WITH_TORQUE             
   SIM_FOC_M:  FOC CPR    :   1000         |  FOC PPR:      2    
   SIM_FOC_M:************ PID Final Status *********
   SIM_FOC_M:  SPEED SP   : 10000.0    |  FLUX SP: 0.0000              |  TORQUE SP: 4.8000        |  FW SP: --
   SIM_FOC_M:  SPEED KP   : 2.7000     |  FLUX KP: 1.0000              |  TORQUE KP: 0.0400        |  FW KP: --
   SIM_FOC_M:  SPEED KI   : 0.0033     |  FLUX KI: 0.0000              |  TORQUE KI: 0.0033        |  FW KI: --
   SIM_FOC_M:  SPEED ERR  : 159.000    |  FLUX ERR:  -0.078            |  TORQUE ERR: 429.0039     |  FW ERR: --
   SIM_FOC_M:  SPEED ACC  : -22210.000 |  FLUX ACC: -318.191           |  TORQUE ACC: -16205.0859  |  FW ACC: --
   SIM_FOC_M:************************************************************************************************************************************
   SIM_FOC_M:********** Simulation parameters ************* Motor parameter ******************** Log files***************************************
   SIM_FOC_M:  Timescale  :     10 (us)    |  motor.w     : 1472.7826  (rad/s) |  Log of parameters : sim_rpm16k.para.foc
   SIM_FOC_M:  Total step :   3000         |  motor.theta :  3.062342  (rad)   |  Log of FOC inputs : sim_rpm16k.in.foc
   SIM_FOC_M:  Total time : 0.030000 (s)   |  motor.Id    : 0.153674   ( A )   |  Log of FOC outputs: sim_rpm16k.out.foc
   SIM_FOC_M:  Inteval    :      3         |  motor.Iq    : -0.000108  ( A ) 
   SIM_FOC_M:  FOC MODE   : MOD_SPEED_WITH_TORQUE             
   SIM_FOC_M:  FOC CPR    :   1000         |  FOC PPR:      2    
   SIM_FOC_M:************ PID Final Status *********
   SIM_FOC_M:  SPEED SP   : 16000.0    |  FLUX SP: 0.0000              |  TORQUE SP: 4.8000        |  FW SP: --
   SIM_FOC_M:  SPEED KP   : 2.7000     |  FLUX KP: 1.0000              |  TORQUE KP: 0.0400        |  FW KP: --
   SIM_FOC_M:  SPEED KI   : 0.0033     |  FLUX KI: 0.0000              |  TORQUE KI: 0.0033        |  FW KI: --
   SIM_FOC_M:  SPEED ERR  : 1936.000   |  FLUX ERR:  -0.152            |  TORQUE ERR: 5225.0000    |  FW ERR: --
   SIM_FOC_M:  SPEED ACC  : -8013.000  |  FLUX ACC: -914.656           |  TORQUE ACC: -31193.4883  |  FW ACC: --
   SIM_FOC_M:************************************************************************************************************************************

* The current test is a hybrid test of the 8 modes run serially.  Each mode's simulation parameter and Motor parameter is shown up.
* For example, the title of "simulation parameters" shows the setting for simulation. The first 3000 test steps
      * use MOD_SPEED_WITH_TORQUE, Speed setpoint is 10000, as we could check the "motor parameter" motor.w=1030.6429 (rad/s) is near the setting rpm 10000. (RPM = motor.w * 60 / (2 * pi) )
      * CPR = 1000 and PPR =2
      * title of "log files" shows the log files generate for this 3000 steps. They will be used as input and golden files for the file flow test, which better simulate the actual running. 
      * Then still apply MOD_SPEED_WITH_TORQUE for the next 3000 steps, Speed setpoint is 16000

.. code-block:: shell

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
   SIM_FOC_F:  VQ        : 24.000000
   SIM_FOC_F:  FW_KP     : 1.000000
   SIM_FOC_F:  FW_KI     : 0.003311
   SIM_FOC_F:  CNT_TRIP  : 3000
   SIM_FOC_F: ***********   Comparison with golden file: ********
   SIM_FOC_F:  Total step  :   3000 
   SIM_FOC_F:  Golden File : sim_torqueWithoutSpeed.out.foc
   SIM_FOC_F:  Max Voltage : 24.000    100.00%
   SIM_FOC_F:  threshold   : 1.200 5.00%
   SIM_FOC_F:  Mean error  : 0.000 0.00%
   SIM_FOC_F:  Max error   : 0.000 0.00%
   SIM_FOC_F:  Min error   : 0.000 0.00%
   SIM_FOC_F:  Total error :      0 
   SIM_FOC_F:********************************************************************************************************************************

waveform explanation and check

1. vim ./xf_motorcontrol/L1/tests/IP_FOC_sensor/ip_foc_periodic_ap_fixed_sim.prj/sol1/csim/build/sim_foc_ModelFoc.log

2. copy the content to excel and draw the speed curve

.. image:: /images/tutorial_sensor_csim.png
   :alt: sensor_csim
   :scale: 100%
   :align: center

COSIM verification

flow 

.. code-block:: shell

   cd ./xf_motorcontrol/L1/tests/IP_FOC_sensor/
   make run COSIM=1  XPART=xc7z020-clg400-1

to see the waveform of signal of the foc, add parameter -wave_debug behind cosim_design in file ./xf_motorcontrol/L1/tests/IP_FOC_sensor/run_hls_sim.tcl  

waveform explanation

   align with 8 modes

.. image:: /images/tutorial_sensor_cosim1.png
   :alt: sensor_cosim1
   :scale: 100%
   :align: center

hls_foc_periodic_top_ap_fixed (ap_fixed version ) cosim with C model motor in mode MOD_SPEED_WITH_TORQUE , "w" of motor is almost stable on 0bbc

.. image:: /images/tutorial_sensor_cosim2.png
   :alt: sensor_cosim2
   :scale: 100%
   :align: center

export IP

   flow

      .. code-block:: shell

         source <Vitis_install_path>/Vitis/2023.1/settings64.sh
         git clone https://github.com/Xilinx/Vitis_Libraries.git
         cd ./motorcontrol/
         git checkout next 
         git pull
         cd L1/tests/FOC/IP_FOC_sensor/
         make run VIVADO_IMPL=1  XPART=xc7z020-clg400-1

   AXI interface

      .. code-block:: shell

         vim ./xf_motorcontrol/L1/tests/IP_FOC_sensor/ip_foc_periodic_ap_fixed_sim.prj/sol1/impl/verilog/hls_foc_periodic_ap_fixed_foc_args_s_axi.v

      .. code-block:: verilog

         // 0x000 : reserved
         // 0x004 : reserved
         // 0x008 : reserved
         // 0x00c : reserved
         // 0x010 : Data signal of ppr_args
         //         bit 31~0 - ppr_args[31:0] (Read/Write)
         // 0x014 : reserved
         // 0x018 : Data signal of control_mode_args
         //         bit 31~0 - control_mode_args[31:0] (Read/Write)
         // 0x01c : reserved
         // 0x020 : Data signal of control_fixperiod_args
         //         bit 31~0 - control_fixperiod_args[31:0] (Read/Write)
         // 0x024 : reserved
         // 0x028 : Data signal of flux_sp_args
         //         bit 31~0 - flux_sp_args[31:0] (Read/Write)
         // 0x02c : reserved
         // 0x030 : Data signal of flux_kp_args
         //         bit 31~0 - flux_kp_args[31:0] (Read/Write)
         // 0x034 : reserved
         // 0x038 : Data signal of flux_ki_args
         //         bit 31~0 - flux_ki_args[31:0] (Read/Write)
         // 0x03c : reserved
         // 0x040 : Data signal of flux_kd_args
         //         bit 31~0 - flux_kd_args[31:0] (Read/Write)
         // 0x044 : reserved
         // 0x048 : Data signal of torque_sp_args
         //         bit 31~0 - torque_sp_args[31:0] (Read/Write)
         // 0x04c : reserved
         // 0x050 : Data signal of torque_kp_args
         //         bit 31~0 - torque_kp_args[31:0] (Read/Write)
         // 0x054 : reserved
         // 0x058 : Data signal of torque_ki_args
         //         bit 31~0 - torque_ki_args[31:0] (Read/Write)
         // 0x05c : reserved
         // 0x060 : Data signal of torque_kd_args
         //         bit 31~0 - torque_kd_args[31:0] (Read/Write)
         // 0x064 : reserved
         // 0x068 : Data signal of speed_sp_args
         //         bit 31~0 - speed_sp_args[31:0] (Read/Write)
         // 0x06c : reserved
         // 0x070 : Data signal of speed_kp_args
         //         bit 31~0 - speed_kp_args[31:0] (Read/Write)
         // 0x074 : reserved
         // 0x078 : Data signal of speed_ki_args
         //         bit 31~0 - speed_ki_args[31:0] (Read/Write)
         // 0x07c : reserved
         // 0x080 : Data signal of speed_kd_args
         //         bit 31~0 - speed_kd_args[31:0] (Read/Write)
         // 0x084 : reserved
         // 0x088 : Data signal of angle_sh_args
         //         bit 31~0 - angle_sh_args[31:0] (Read/Write)
         // 0x08c : reserved
         // 0x090 : Data signal of vd_args
         //         bit 31~0 - vd_args[31:0] (Read/Write)
         // 0x094 : reserved
         // 0x098 : Data signal of vq_args
         //         bit 31~0 - vq_args[31:0] (Read/Write)
         // 0x09c : reserved
         // 0x0a0 : Data signal of fw_kp_args
         //         bit 31~0 - fw_kp_args[31:0] (Read/Write)
         // 0x0a4 : reserved
         // 0x0a8 : Data signal of fw_ki_args
         //         bit 31~0 - fw_ki_args[31:0] (Read/Write)
         // 0x0ac : reserved
         // 0x0b0 : Data signal of id_stts
         //         bit 31~0 - id_stts[31:0] (Read)
         // 0x0b4 : reserved
         // 0x0c0 : Data signal of flux_acc_stts
         //         bit 31~0 - flux_acc_stts[31:0] (Read)
         // 0x0c4 : reserved
         // 0x0d0 : Data signal of flux_err_stts
         //         bit 31~0 - flux_err_stts[31:0] (Read)
         // 0x0d4 : reserved
         // 0x0e0 : Data signal of flux_out_stts
         //         bit 31~0 - flux_out_stts[31:0] (Read)
         // 0x0e4 : reserved
         // 0x0f0 : Data signal of iq_stts
         //         bit 31~0 - iq_stts[31:0] (Read)
         // 0x0f4 : reserved
         // 0x100 : Data signal of torque_acc_stts
         //         bit 31~0 - torque_acc_stts[31:0] (Read)
         // 0x104 : reserved
         // 0x110 : Data signal of torque_err_stts
         //         bit 31~0 - torque_err_stts[31:0] (Read)
         // 0x114 : reserved
         // 0x120 : Data signal of torque_out_stts
         //         bit 31~0 - torque_out_stts[31:0] (Read)
         // 0x124 : reserved
         // 0x130 : Data signal of speed_stts
         //         bit 31~0 - speed_stts[31:0] (Read)
         // 0x134 : reserved
         // 0x140 : Data signal of speed_acc_stts
         //         bit 31~0 - speed_acc_stts[31:0] (Read)
         // 0x144 : reserved
         // 0x150 : Data signal of speed_err_stts
         //         bit 31~0 - speed_err_stts[31:0] (Read)
         // 0x154 : reserved
         // 0x160 : Data signal of speed_out_stts
         //         bit 31~0 - speed_out_stts[31:0] (Read)
         // 0x164 : reserved
         // 0x170 : Data signal of angle_stts
         //         bit 31~0 - angle_stts[31:0] (Read)
         // 0x174 : reserved
         // 0x180 : Data signal of Va_cmd_stts
         //         bit 31~0 - Va_cmd_stts[31:0] (Read)
         // 0x184 : reserved
         // 0x190 : Data signal of Vb_cmd_stts
         //         bit 31~0 - Vb_cmd_stts[31:0] (Read)
         // 0x194 : reserved
         // 0x1a0 : Data signal of Vc_cmd_stts
         //         bit 31~0 - Vc_cmd_stts[31:0] (Read)
         // 0x1a4 : reserved
         // 0x1b0 : Data signal of Ialpha_stts
         //         bit 31~0 - Ialpha_stts[31:0] (Read)
         // 0x1b4 : reserved
         // 0x1c0 : Data signal of Ibeta_stts
         //         bit 31~0 - Ibeta_stts[31:0] (Read)
         // 0x1c4 : reserved
         // 0x1d0 : Data signal of Ihomopolar_stts
         //         bit 31~0 - Ihomopolar_stts[31:0] (Read)
         // 0x1d4 : reserved
         // (SC = Self Clear, COR = Clear on Read, TOW = Toggle on Write, COH = Clear on Handshake)

Lab-3: Simulation and verification of FOC_sensorless
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

CSIM verification

flow

.. code-block:: shell

   source <Vitis_install_path>/Vitis/2023.1/settings64.sh
   git clone https://github.com/Xilinx/Vitis_Libraries.git
   cd ./motorcontrol/
   git checkout next 
   git pull
   cd L1/tests/IP_FOC_sensorless/
   make run CSIM=1  XPART=xc7z020-clg400-1

stdout explanation

.. code-block:: shell

   vim ./xf_motorcontrol/L1/tests/IP_FOC_sensorless/ip_foc_periodic_sensorless_ap_fixed.prj/sol1/csim/report/hls_foc_periodic_ap_fixed_csim.log

   SIM_FOC_M:********** Simulation parameters ************* Motor parameter ******************** Log files***************************************
   SIM_FOC_M:  Timescale  :      0 (us)    |  motor.w     : 716.1517   (rad/s) |  Log of parameters : sim_torqueWithoutSpeed.para.foc
   SIM_FOC_M:  Total step : 300000         |  motor.theta :  4.984835  (rad)   |  Log of FOC inputs : sim_torqueWithoutSpeed.in.foc
   SIM_FOC_M:  Total time : 0.030000 (s)   |  motor.Id    : 0.342496   ( A )   |  Log of FOC outputs: sim_torqueWithoutSpeed.out.foc
   SIM_FOC_M:  Inteval    :    300         |  motor.Iq    : 1.253088   ( A ) 
   SIM_FOC_M:  FOC MODE   : MOD_TORQUE_WITHOUT_SPEED          
   SIM_FOC_M:  FOC CPR    :   1000         |  FOC PPR:      2    
   SIM_FOC_M:************ PID Final Status *********
   SIM_FOC_M:  SPEED SP   : 10000.0    |  FLUX SP: 0.0000              |  TORQUE SP: 4.8000        |  FW SP: --
   SIM_FOC_M:  SPEED KP   : 2.7000     |  FLUX KP: 1.0000              |  TORQUE KP: 5.0000        |  FW KP: --
   SIM_FOC_M:  SPEED KI   : 0.0033     |  FLUX KI: 0.0000              |  TORQUE KI: 0.0033        |  FW KI: --
   SIM_FOC_M:  SPEED ERR  : 3024.000   |  FLUX ERR:  -0.320            |  TORQUE ERR:  3.5391  |  FW ERR: --
   SIM_FOC_M:  SPEED ACC  : -1.000 |  FLUX ACC: -17204.664         |  TORQUE ACC: 11541.7734   |  FW ACC: --
   SIM_FOC_M:************************************************************************************************************************************
   SIM_FOC_M:********** Simulation parameters ************* Motor parameter ******************** Log files***************************************
   SIM_FOC_M:  Timescale  :      0 (us)    |  motor.w     : 1024.1533  (rad/s) |  Log of parameters : sim_rpm10k.para.foc
   SIM_FOC_M:  Total step : 300000         |  motor.theta :  4.085604  (rad)   |  Log of FOC inputs : sim_rpm10k.in.foc
   SIM_FOC_M:  Total time : 0.030000 (s)   |  motor.Id    : 0.077624   ( A )   |  Log of FOC outputs: sim_rpm10k.out.foc
   SIM_FOC_M:  Inteval    :    300         |  motor.Iq    : 0.026773   ( A ) 
   SIM_FOC_M:  FOC MODE   : MOD_SPEED_WITH_TORQUE             
   SIM_FOC_M:  FOC CPR    :   1000         |  FOC PPR:      2    
   SIM_FOC_M:************ PID Final Status *********
   SIM_FOC_M:  SPEED SP   : 10000.0    |  FLUX SP: 0.0000              |  TORQUE SP: 4.8000        |  FW SP: --
   SIM_FOC_M:  SPEED KP   : 2.7000     |  FLUX KP: 1.0000              |  TORQUE KP: 0.0400        |  FW KP: --
   SIM_FOC_M:  SPEED KI   : 0.0033     |  FLUX KI: 0.0000              |  TORQUE KI: 0.0033        |  FW KI: --
   SIM_FOC_M:  SPEED ERR  : 311.000    |  FLUX ERR:  -0.074            |  TORQUE ERR: 838.9727     |  FW ERR: --
   SIM_FOC_M:  SPEED ACC  : -1.000 |  FLUX ACC: -30529.977         |  TORQUE ACC: -0.0039  |  FW ACC: --
   SIM_FOC_M:************************************************************************************************************************************
   SIM_FOC_M:********** Simulation parameters ************* Motor parameter ******************** Log files***************************************
   SIM_FOC_M:  Timescale  :      0 (us)    |  motor.w     : 1474.3268  (rad/s) |  Log of parameters : sim_rpm16k.para.foc
   SIM_FOC_M:  Total step : 300000         |  motor.theta :  2.892733  (rad)   |  Log of FOC inputs : sim_rpm16k.in.foc
   SIM_FOC_M:  Total time : 0.030000 (s)   |  motor.Id    : 0.142991   ( A )   |  Log of FOC outputs: sim_rpm16k.out.foc
   SIM_FOC_M:  Inteval    :    300         |  motor.Iq    : 0.000147   ( A ) 
   SIM_FOC_M:  FOC MODE   : MOD_SPEED_WITH_TORQUE             
   SIM_FOC_M:  FOC CPR    :   1000         |  FOC PPR:      2    
   SIM_FOC_M:************ PID Final Status *********
   SIM_FOC_M:  SPEED SP   : 16000.0    |  FLUX SP: 0.0000              |  TORQUE SP: 4.8000        |  FW SP: --
   SIM_FOC_M:  SPEED KP   : 2.7000     |  FLUX KP: 1.0000              |  TORQUE KP: 0.0400        |  FW KP: --
   SIM_FOC_M:  SPEED KI   : 0.0033     |  FLUX KI: 0.0000              |  TORQUE KI: 0.0033        |  FW KI: --
   SIM_FOC_M:  SPEED ERR  : 1899.000   |  FLUX ERR:  -0.141            |  TORQUE ERR: 5124.9961    |  FW ERR: --
   SIM_FOC_M:  SPEED ACC  : -1.000 |  FLUX ACC: -21425.285         |  TORQUE ACC: -0.0039  |  FW ACC: --
   SIM_FOC_M:************************************************************************************************************************************

* The current test is a hybrid test of the three modes run serially.  Each mode's simulation parameter and Motor parameter is shown up.
* For example, the title of "simulation parameters" shows the setting for simulation. The first 300000 test steps
   * use MOD_SPEED_WITH_TORQUE, Speed setpoint is 10000, as we could check the "motor parameter" motor.w=1024.1533 (rad/s) is near the setting rpm 10000. (RPM = motor.w * 60 / (2 * pi) )
   * CPR = 1000 and PPR =2
   * title of "log files" shows the log files generate for this 300000 steps. They will be used as input and golden files for the file flow test, which better simulate the actual running. 
   * Then still apply MOD_SPEED_WITH_TORQUE for the next 300000 steps, Speed setpoint is 16000

.. code-block:: shell

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
   SIM_FOC_F:  VQ        : 24.000000
   SIM_FOC_F:  FW_KP     : 1.000000
   SIM_FOC_F:  FW_KI     : 0.003311
   SIM_FOC_F:  CNT_TRIP  : 300000
   SIM_FOC_F: ***********   Comparison with golden file: ********
   SIM_FOC_F:  Total step  : 300000 
   SIM_FOC_F:  Golden File : sim_torqueWithoutSpeed.out.foc
   SIM_FOC_F:  Max Voltage : 24.000    100.00%
   SIM_FOC_F:  threshold   : 1.200 5.00%
   SIM_FOC_F:  Mean error  : 0.000 0.00%
   SIM_FOC_F:  Max error   : 0.000 0.00%
   SIM_FOC_F:  Min error   : 0.000 0.00%
   SIM_FOC_F:  Total error :      0 
   SIM_FOC_F:********************************************************************************************************************************

waveform explanation and check

1. vim ./xf_motorcontrol/L1/tests/IP_FOC_sensorless/ip_foc_periodic_sensorless_ap_fixed.prj/sol1/csim/build/sim_foc_ModelFoc.log

2. copy the content to excel and draw the speed curve

.. image:: /images/tutorial_sensorless_csim.png
   :alt: sensorless_csim
   :scale: 100%
   :align: center

COSIM verification 

flow 

.. code-block:: shell

   cd ./xf_motorcontrol/L1/tests/IP_FOC_sensorless/
   make run COSIM=1  XPART=xc7z020-clg400-1

to see the waveform of signal of the foc, add parameter -wave_debug behind cosim_design in file ./xf_motorcontrol/L1/tests/IP_FOC_sensor/run_hls_sim.tcl

waveform explanation

align with 8 modes

.. image:: /images/tutorial_sensorless_cosim.png
   :alt: sensorless_cosim
   :scale: 100%
   :align: center

export IP

   flow

      .. code-block:: shell

         source <Vitis_install_path>/Vitis/2023.1/settings64.sh
         git clone https://github.com/Xilinx/Vitis_Libraries.git
         cd ./motorcontrol/
         git checkout next 
         git pull
         cd L1/tests/FOC/IP_FOC_sensorless/
         make run VIVADO_IMPL=1  XPART=xc7z020-clg400-1

   AXI interface

      .. code-block:: shell

         vim ./xf_motorcontrol/L1/tests/IP_FOC_sensorless/ip_foc_periodic_sensorless_ap_fixed.prj/sol1/impl/verilog/hls_foc_periodic_ap_fixed_foc_args_s_axi.v

      .. code-block:: verilog

         // 0x000 : reserved
         // 0x004 : reserved
         // 0x008 : reserved
         // 0x00c : reserved
         // 0x010 : Data signal of ppr_args
         //         bit 31~0 - ppr_args[31:0] (Read/Write)
         // 0x014 : reserved
         // 0x018 : Data signal of control_mode_args
         //         bit 31~0 - control_mode_args[31:0] (Read/Write)
         // 0x01c : reserved
         // 0x020 : Data signal of control_fixperiod_args
         //         bit 31~0 - control_fixperiod_args[31:0] (Read/Write)
         // 0x024 : reserved
         // 0x028 : Data signal of flux_sp_args
         //         bit 31~0 - flux_sp_args[31:0] (Read/Write)
         // 0x02c : reserved
         // 0x030 : Data signal of flux_kp_args
         //         bit 31~0 - flux_kp_args[31:0] (Read/Write)
         // 0x034 : reserved
         // 0x038 : Data signal of flux_ki_args
         //         bit 31~0 - flux_ki_args[31:0] (Read/Write)
         // 0x03c : reserved
         // 0x040 : Data signal of flux_kd_args
         //         bit 31~0 - flux_kd_args[31:0] (Read/Write)
         // 0x044 : reserved
         // 0x048 : Data signal of torque_sp_args
         //         bit 31~0 - torque_sp_args[31:0] (Read/Write)
         // 0x04c : reserved
         // 0x050 : Data signal of torque_kp_args
         //         bit 31~0 - torque_kp_args[31:0] (Read/Write)
         // 0x054 : reserved
         // 0x058 : Data signal of torque_ki_args
         //         bit 31~0 - torque_ki_args[31:0] (Read/Write)
         // 0x05c : reserved
         // 0x060 : Data signal of torque_kd_args
         //         bit 31~0 - torque_kd_args[31:0] (Read/Write)
         // 0x064 : reserved
         // 0x068 : Data signal of speed_sp_args
         //         bit 31~0 - speed_sp_args[31:0] (Read/Write)
         // 0x06c : reserved
         // 0x070 : Data signal of speed_kp_args
         //         bit 31~0 - speed_kp_args[31:0] (Read/Write)
         // 0x074 : reserved
         // 0x078 : Data signal of speed_ki_args
         //         bit 31~0 - speed_ki_args[31:0] (Read/Write)
         // 0x07c : reserved
         // 0x080 : Data signal of speed_kd_args
         //         bit 31~0 - speed_kd_args[31:0] (Read/Write)
         // 0x084 : reserved
         // 0x088 : Data signal of angle_sh_args
         //         bit 31~0 - angle_sh_args[31:0] (Read/Write)
         // 0x08c : reserved
         // 0x090 : Data signal of vd_args
         //         bit 31~0 - vd_args[31:0] (Read/Write)
         // 0x094 : reserved
         // 0x098 : Data signal of vq_args
         //         bit 31~0 - vq_args[31:0] (Read/Write)
         // 0x09c : reserved
         // 0x0a0 : Data signal of fw_kp_args
         //         bit 31~0 - fw_kp_args[31:0] (Read/Write)
         // 0x0a4 : reserved
         // 0x0a8 : Data signal of fw_ki_args
         //         bit 31~0 - fw_ki_args[31:0] (Read/Write)
         // 0x0ac : reserved
         // 0x0b0 : Data signal of id_stts
         //         bit 31~0 - id_stts[31:0] (Read)
         // 0x0b4 : reserved
         // 0x0c0 : Data signal of flux_acc_stts
         //         bit 31~0 - flux_acc_stts[31:0] (Read)
         // 0x0c4 : reserved
         // 0x0d0 : Data signal of flux_err_stts
         //         bit 31~0 - flux_err_stts[31:0] (Read)
         // 0x0d4 : reserved
         // 0x0e0 : Data signal of flux_out_stts
         //         bit 31~0 - flux_out_stts[31:0] (Read)
         // 0x0e4 : reserved
         // 0x0f0 : Data signal of iq_stts
         //         bit 31~0 - iq_stts[31:0] (Read)
         // 0x0f4 : reserved
         // 0x100 : Data signal of torque_acc_stts
         //         bit 31~0 - torque_acc_stts[31:0] (Read)
         // 0x104 : reserved
         // 0x110 : Data signal of torque_err_stts
         //         bit 31~0 - torque_err_stts[31:0] (Read)
         // 0x114 : reserved
         // 0x120 : Data signal of torque_out_stts
         //         bit 31~0 - torque_out_stts[31:0] (Read)
         // 0x124 : reserved
         // 0x130 : Data signal of speed_stts
         //         bit 31~0 - speed_stts[31:0] (Read)
         // 0x134 : reserved
         // 0x140 : Data signal of speed_acc_stts
         //         bit 31~0 - speed_acc_stts[31:0] (Read)
         // 0x144 : reserved
         // 0x150 : Data signal of speed_err_stts
         //         bit 31~0 - speed_err_stts[31:0] (Read)
         // 0x154 : reserved
         // 0x160 : Data signal of speed_out_stts
         //         bit 31~0 - speed_out_stts[31:0] (Read)
         // 0x164 : reserved
         // 0x170 : Data signal of angle_stts
         //         bit 31~0 - angle_stts[31:0] (Read)
         // 0x174 : reserved
         // 0x180 : Data signal of Va_cmd_stts
         //         bit 31~0 - Va_cmd_stts[31:0] (Read)
         // 0x184 : reserved
         // 0x190 : Data signal of Vb_cmd_stts
         //         bit 31~0 - Vb_cmd_stts[31:0] (Read)
         // 0x194 : reserved
         // 0x1a0 : Data signal of Vc_cmd_stts
         //         bit 31~0 - Vc_cmd_stts[31:0] (Read)
         // 0x1a4 : reserved
         // 0x1b0 : Data signal of Ialpha_stts
         //         bit 31~0 - Ialpha_stts[31:0] (Read)
         // 0x1b4 : reserved
         // 0x1c0 : Data signal of Ibeta_stts
         //         bit 31~0 - Ibeta_stts[31:0] (Read)
         // 0x1c4 : reserved
         // 0x1d0 : Data signal of Ihomopolar_stts
         //         bit 31~0 - Ihomopolar_stts[31:0] (Read)
         // 0x1d4 : reserved
         // (SC = Self Clear, COR = Clear on Read, TOW = Toggle on Write, COH = Clear on Handshake)

Lab-4: Simulation and verification of SVPWM-DUTY
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

CSIM verification

flow

   .. code-block:: shell

      source <Vitis_install_path>/Vitis/2023.1/settings64.sh
      git clone https://github.com/Xilinx/Vitis_Libraries.git
      cd xf_motorcontrol/
      git checkout next 
      git pull
      cd L1/tests/IP_SVPWM_DUTY/
      make run CSIM=1  XPART=xc7z020-clg400-1

   execute the executable file with parameters follow the step below:

   .. code-block:: shell

      cd ./xf_motorcontrol/L1/tests/IP_SVPWM_DUTY/svpwm_duty_sim.prj/sol1/csim/build
      ./csim.exe condition: [-shift_0/-shift_120] | [-dc_adc/-dc_ref] | [-pwm_fq <pwm frequency>] | [-dead <dead cycles>] [-ii <sampling II>] [-cnt <Number of input>]   

stdout explanation

.. image:: /images/tutorial_svpwm_duty_stdout.png
   :alt: svpwm_duty_stdout
   :scale: 100%
   :align: center

The stdout has shown the AXI-lite signal and inner status monitoring parameters.

* ``stt_cnt_iter``: is used to monitor the inner trip count of svpwm_duty.  It can register how many inputs are consumed totally. The default value is 10.
* ``args_dc_link_ref``: is supposed to connect with the external direct-current voltage source for reference. Typically, if the magnitude of the synthesized electric vector is smaller than the value, the high voltage duration of the switch pair should work longer. The duration should multiply a compensate-ratio factor.
* ``args_dc_src_mode``: decides where the external source of dc_link voltage for reference is coming from. 0, the dc_link reference voltage is determined by the measured value of the ADC dc_link voltage; 1, the reference voltage is solely determined by the preset static register value for reference voltage.
* ``args_sample_ii``: it can control the consumption rate of SVPWM_DUTY. The svpwm is producing the waveform every cycle. The default sample-ii value is 1. 
   1. In the CSIM, the fifo is assumed to be infinitely large. Thus, all of the 10 inputs are able to be stacked into the fifo and produce exactly the same number of outputs. Thus, no matter of what sample-ii value is here, the CSIM result always remains same here.
   2. In the COSIM, since we define the fifo depth as 4, the fifo can only take in the maximum number of 4 inputs. If the sample-ii is 1 clock cycle, while the downstream of PWM_GEN is consuming the data every 1000 cycles, the input fifo is soon stuffed full of 4 inputs and other 6 inputs are discarded immediately. However, if the sample-ii is set as 1000 clock cycles, every data will consume at a rate of 1000. It can guarantee that no data is going to be discarded. The COSIM behavior complies with the CSIM.
* ``dc_link_adc``: is the measured value of ADC dc_link value.
* ``V_ref``: is rounded value of dc_link_adc
* ``ratio_compensate``: multiply with the duty_cycle in each channel to compensate the final synthesized voltage.

waveform explanation and check

1. vim ./xf_motorcontrol/L1/tests/IP_SVPWM_DUTY/svpwm_duty_sim.prj/sol1/csim/build/wave_all10
2. copy the content to excel and draw the curve

phase_shift_mode = MODE_PWM_PHASE_SHIFT::SHIFT_ZERO

.. image:: /images/tutorial_svpwm_duty_csim_shiftzero.png
   :alt: svpwm_duty_csim_shiftzero
   :scale: 100%
   :align: center

phase_shift_mode = MODE_PWM_PHASE_SHIFT::SHIFT_120

.. image:: /images/tutorial_svpwm_duty_csim_shift120.png
   :alt: svpwm_duty_csim_shift120
   :scale: 100%
   :align: center

COSIM verification

flow

.. code-block:: shell

   make run COSIM=1 XPART=xc7z020-clg400-1

waveform explanation

Verify latency from input to output.

.. image:: /images/tutorial_svpwm_duty_cosim1.png
   :alt: svpwm_duty_cosim1
   :scale: 100%
   :align: center

According to waveform, the measured latency is 504.750ns from sampling commands to outputting ratios (50 cycles)

Verify latency of pwm_cycle 

.. image:: /images/tutorial_svpwm_duty_cosim2.png
   :alt: svpwm_duty_cosim2
   :scale: 100%
   :align: center

ii-sample's impact
Testcase [-v0]

.. code-block:: cpp

   float Va0_f[TESTNUMBER] = {0,    7.406250,    13.968750,    8.250000,    -7.968750,    -14.062500,    -7.312500,    6.187500,    MAX_VAL_PWM,    -MAX_VAL_PWM};
   float Vb0_f[TESTNUMBER] = {0,    -14.062500,    -7.593750,    5.625000,    13.968750,    6.656250,    -6.750000,    -14.062500,    MAX_VAL_PWM,    -MAX_VAL_PWM};                                        
   float Vc0_f[TESTNUMBER] = {0,    6.5625,    -6.46875,    -13.96875,    -6.09375,    7.21875,    13.96875,    7.78125,    MAX_VAL_PWM,    -MAX_VAL_PWM};

Testcase [-v2]

.. code-block:: cpp

   t_svpwm_cmd Va2[TESTNUMBER] = {24, 22, 18, 12,  6,  3,  0,-12,-18,-24};                    
   t_svpwm_cmd Vb2[TESTNUMBER] = { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0};                                        
   t_svpwm_cmd Vc2[TESTNUMBER] = { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0};  

Test Case [-v0] default

ii-sample= 1

The fifo depth is 4 and each iteration latency is 93 clock cycles.  Each output is programmed to come out at an interval of 1000 clock cycles. f the sample-ii is 1, the fifo is soon filled full of the data (within 93 clock cycles) while the first output is still not coming yet.  Another 6 data are immediately discarded. The following diagram depicts that the final outcome only has 4 duty_cycles.

.. image:: /images/tutorial_svpwm_duty_cosim_iisample1.png
   :alt: svpwm_duty_cosim_iisample1
   :scale: 100%
   :align: center

ii-sample= 50

The fifo depth is 4 and each iteration latency is 93 clock cycles. When ii-sample is 50, one signal is consumed and the result comes after 93. The fifo has enough space to store the next two stream-in data. Hence the output end can produce the complete ten waveform.

.. image:: /images/tutorial_svpwm_duty_cosim_iisample50.png
   :alt: svpwm_duty_cosim_iisample50
   :scale: 100%
   :align: center

ii-sample= 1000 (Default)

.. image:: /images/tutorial_svpwm_duty_cosim_iisample1000.png
   :alt: svpwm_duty_cosim_iisample1000
   :scale: 100%
   :align: center

export IP

   flow

      .. code-block:: shell

         source <Vitis_install_path>/Vitis/2022.2/settings64.sh
         git clone https://github.com/Xilinx/Vitis_Libraries.git
         cd xf_motorcontrol/
         git checkout next 
         git pull
         cd L1/tests/FOC/IP_SVPWM/
         make run VIVADO_IMPL=1  XPART=xc7z020-clg400-1

   AXI interface

      .. code-block:: shell

         vim ./xf_motorcontrol/L1/tests/IP_SVPWM_DUTY/svpwm_hls_lib.prj/sol1/impl/verilog/hls_svpwm_duty_pwm_args_s_axi.v

      .. code-block:: verilog
         // 0x00 : reserved
         // 0x04 : reserved
         // 0x08 : reserved
         // 0x0c : reserved
         // 0x10 : Data signal of pwm_args_dc_link_ref
         //        bit 31~0 - pwm_args_dc_link_ref[31:0] (Read/Write)
         // 0x14 : reserved
         // 0x18 : Data signal of pwm_stt_cnt_iter
         //        bit 31~0 - pwm_stt_cnt_iter[31:0] (Read)
         // 0x1c : reserved
         // 0x28 : Data signal of pwm_args_dc_src_mode
         //        bit 31~0 - pwm_args_dc_src_mode[31:0] (Read/Write)
         // 0x2c : reserved
         // 0x30 : Data signal of pwm_args_sample_ii
         //        bit 31~0 - pwm_args_sample_ii[31:0] (Read/Write)
         // 0x34 : reserved
         // 0x38 : Data signal of pwm_stt_Va_cmd
         //        bit 31~0 - pwm_stt_Va_cmd[31:0] (Read)
         // 0x3c : reserved
         // 0x48 : Data signal of pwm_stt_Vb_cmd
         //        bit 31~0 - pwm_stt_Vb_cmd[31:0] (Read)
         // 0x4c : reserved
         // 0x58 : Data signal of pwm_stt_Vc_cmd
         //        bit 31~0 - pwm_stt_Vc_cmd[31:0] (Read)
         // 0x5c : reserved
         // (SC = Self Clear, COR = Clear on Read, TOW = Toggle on Write, COH = Clear on Handshake)

Lab-5: Simulation and verification of PWM-GEN
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

CSIM verification

flow

.. code-block:: shell

   source <Vitis_install_path>/Vitis/2023.1/settings64.sh
   git clone https://github.com/Xilinx/Vitis_Libraries.git
   cd ./xf_motorcontrol/
   git checkout next 
   git pull
   cd L1/tests/IP_PWM_GEN/
   make run CSIM=1  XPART=xc7z020-clg400-1

stdout explanation

.. image:: /images/tutorial_pwm_gen_stdout.png
   :alt: pwm_gen_stdout
   :scale: 100%
   :align: center

The stdout shown above depicts the following info:

* ``stt_pwm_cycle``: the pwm_cycle is defined as the ``clock_freq / args_pwm_freq``, which is supposed to have a default value of 1000. Typically, it can help to test and verify the RTL behavior.
* ``args_pwm_freq``: the pwm frequency is the wave frequency after the pulse width modulation. Within every pwm cycle = ``[1 / args_pwm_freq]``, the high voltage duration can be modulated as any value within the range of 0~pwm_cycle.
* ``args_dead_cycles``: the upper and lower switches on the same bridge cannot switch simultaneously, otherwise the overloaded transient current on this branch will cause irrevertible damage to the system.  Thus, for every complementary swtich pair, the upper switch will react a dead_cycles times ahead of the lower switch.
* ``args_phase_shift``: it determines the phase_shift mode. 0, there is no phase_shift in the gating and sync. 1, there is 120-degree phase_shift in the svpwm.
* ``args_sample_ii``: it determines the consumption rate of hls_pwm_gen. The default consumption rate is ii = 1000. 
   1. In the CSIM, the fifo is assumed to be infinitely large. Thus, all of the 10 inputs are able to be stacked into the fifo and produce exactly the same number of outputs. Thus, no matter of what sample-ii value is here, the CSIM result always remains same here.
   2. In the COSIM, the default fifo depth is 2.  The hls_pwm_gen consumes every input and produces 1000 output, with a pipeline interval of 1 clock cycle. Hence ii > 1000 will have the previous input produce the output twice.  The COSIM -ii=2000 diagram depicts the phenomenon.

COSIM verification

flow

   waveform explanation

   -ii = 1000

   complete 10 waveform.

.. image:: /images/tutorial_pwm_gen_cosim1.png
   :alt: pwm_gen_cosim1
   :scale: 100%
   :align: center

-ii = 2000

the hls_pwm_gen read once at the beginning, so produce the exact waveform as [-ii = 1000]. However, since the sampling rate is set as [-ii = 2000], the wave produces as the usual rate., the wave after 1 is producing the wave twice.

.. image:: /images/tutorial_pwm_gen_cosim2.png
   :alt: pwm_gen_cosim2
   :scale: 100%
   :align: center

export IP

   flow

      .. code-block:: shell

         source <Vitis_install_path>/Vitis/2022.2/settings64.sh
         git clone https://github.com/Xilinx/Vitis_Libraries.git
         cd ./motorcontrol/
         git checkout next 
         git pull
         cd L1/tests/IP_PWM_GEN/
         make run VIVADO_IMPL=1  XPART=xc7z020-clg400-1

   AXI interface

      .. code-block:: shell

         vim ./xf_motorcontrol/L1/tests/IP_PWM_GEN/pwm_gen.prj/sol1/impl/verilog/hls_pwm_gen_pwm_args_s_axi.v

      .. code-block:: verilog

         // 0x00 : reserved
         // 0x04 : reserved
         // 0x08 : reserved
         // 0x0c : reserved
         // 0x10 : Data signal of pwm_args_pwm_freq
         //        bit 31~0 - pwm_args_pwm_freq[31:0] (Read/Write)
         // 0x14 : reserved
         // 0x18 : Data signal of pwm_args_dead_cycles
         //        bit 31~0 - pwm_args_dead_cycles[31:0] (Read/Write)
         // 0x1c : reserved
         // 0x20 : Data signal of pwm_args_phase_shift
         //        bit 31~0 - pwm_args_phase_shift[31:0] (Read/Write)
         // 0x24 : reserved
         // 0x28 : Data signal of pwm_stt_pwm_cycle
         //        bit 31~0 - pwm_stt_pwm_cycle[31:0] (Read)
         // 0x2c : reserved
         // 0x38 : Data signal of pwm_args_sample_ii
         //        bit 31~0 - pwm_args_sample_ii[31:0] (Read/Write)
         // 0x3c : reserved
         // 0x40 : Data signal of pwm_stt_duty_ratio_a
         //        bit 31~0 - pwm_stt_duty_ratio_a[31:0] (Read)
         // 0x44 : reserved
         // 0x50 : Data signal of pwm_stt_duty_ratio_b
         //        bit 31~0 - pwm_stt_duty_ratio_b[31:0] (Read)
         // 0x54 : reserved
         // 0x60 : Data signal of pwm_stt_duty_ratio_c
         //        bit 31~0 - pwm_stt_duty_ratio_c[31:0] (Read)
         // 0x64 : reserved
         // (SC = Self Clear, COR = Clear on Read, TOW = Toggle on Write, COH = Clear on Handshake)

Lab-6: Simulation and verification of QEI
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

CSIM verification

flow

.. code-block:: shell

   cd ./xf_motorcontrol/L1/tests/IP_QEI/
   make run CSIM=1  XPART=xc7z020-clg400-1

stdout explanation

.. image:: /images/tutorial_qei_stdout.png
   :alt: qei_stdout
   :scale: 100%
   :align: center

* The title of "Generating Input" shows the setting for input args
   * CLK=100M
   * CPR=1000
   * rmp={3000, 1000, 2000, 5000, 100, 200, 300},
   * angle_start={355.0, 54.7, 45.0, 25.2, 5.4, 7.2, 9.7}
   * dir={1, 0, 0, 0, 1, 1, 1}, corresponding angle run={60, 10, 20, 20, 2, 3, 5}, corresponding steps={166, 27, 55, 55, 5, 8, 13}
   * freq_AB={50.0k, 16.7k, 33.3k, 83.3k, 1.7k, 5.0k, 8.3k}, the freq_AB from formula freq_AB=rpm / 60.0 * cpr
   * num_write={332000, 161892, 164780, 66000, 299980, 160000, 155948}: the total number of ABI signals each running
   * time_used={0.003320, 0.001619, 0.001648, 0.000660, 0.003000, 0.001600, 0.001559}: the time used of system simulation each running
   * rpm_est={3012, 1029, 2023, 5051, 111, 313, 534}: the value from formula time_used = (float)num_write/(float)freq_clk 
* The title of "Output analysis" shows the output of simulation 
   * Changed out: the number of output 
   * dir: the output of dir should be the same with dir of input setting
   * rpm: the output of rpm should fuzzy match the input dir
   * angle_detected: the angle_detected should fuzzy match the input angle_start
   * counter: the number of steps required for the current location or angle
   * err: 0 represents valid output, 3 represents invalid output
* The title of "AXI Parameter Value" shows the axi-port of return

COSIM verification

flow

.. code-block:: shell

   cd ./xf_motorcontrol/L1/tests/IP_QEI/
   make run COSIM=1  XPART=xc7z020-clg400-1

waveform explanation

.. image:: /images/tutorial_qei_cosim.png
   :alt: qei_cosim
   :scale: 100%
   :align: center

First, we can check if the input signals including A, B, I are valid, and they should be same to the waveform above like high and low level interaction. Moreover, the output of qei_err_TDATA indicates whether the input signal is valid, just 0 represents valid input and 3 represents invalid input.

Secondly, we should focus on the output for qei_RPM_THETA_m_TDATA, it includes speed(rpm) and angle(theta). The rpm will be a stable result like "0bb8" as shown on waveform above, and it should be equal to your setting given by user. The angle will be a increasing value like "0001 0002 ... " as shown on waveform above, and it depends on PPR of motor system.

Finally, the axi-lite of port should be update status value for clocks just like waveform shown above. 

export IP

   flow

      .. code-block:: shell

         cd ./xf_motorcontrol/L1/tests/IP_QEI/
         make run VIVADO_SYN=1 VIVADO_IMPL=1 XPART=xc7z020-clg400-1

   AXI interface

      .. code-block:: shell

         vim ./xf_motorcontrol/L1/tests/IP_QEI/hls_qei.prj/sol1/impl/verilog/hls_qei_qei_args_s_axi.v

      .. code-block:: verilog

         // 0x00 : reserved
         // 0x04 : reserved
         // 0x08 : reserved
         // 0x0c : reserved
         // 0x10 : Data signal of qei_args_cpr_i
         //        bit 31~0 - qei_args_cpr_i[31:0] (Read/Write)
         // 0x14 : reserved
         // 0x18 : Data signal of qei_args_cpr_o
         //        bit 31~0 - qei_args_cpr_o[31:0] (Read)
         // 0x1c : reserved
         // 0x20 : Data signal of qei_args_ctrl
         //        bit 31~0 - qei_args_ctrl[31:0] (Read/Write)
         // 0x24 : reserved
         // 0x28 : Data signal of qei_stts_RPM_THETA_m
         //        bit 31~0 - qei_stts_RPM_THETA_m[31:0] (Read)
         // 0x2c : reserved
         // 0x38 : Data signal of qei_stts_dir
         //        bit 31~0 - qei_stts_dir[31:0] (Read)
         // 0x3c : reserved
         // 0x48 : Data signal of qei_stts_err
         //        bit 31~0 - qei_stts_err[31:0] (Read)
         // 0x4c : reserved
         // (SC = Self Clear, COR = Clear on Read, TOW = Toggle on Write, COH = Clear on Handshake)


For more information about how to analyze performance, please refer to `Application Acceleration Development (UG1393) <https://docs.xilinx.com/r/2020.2-English/ug1393-vitis-application-acceleration/Profiling-Optimizing-and-Debugging-the-Application>`_


Lab summary
^^^^^^^^^^^^^^

* Focus on Motor Control

Tutorial Summary
------------------

.. toctree::
   :maxdepth: 2

   L1 Benchmark <benchmark.rst>

The tutorial will be developed to cover more Motor contorl motheds and their combinations, more flows and more classic applications.
