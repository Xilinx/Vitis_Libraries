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

.. meta::
   :keywords: Vitis, Motor Control, Vitis Motor Control Library, Alveo
   :description: AMD Vitis |trade| Motor Control Library is an open-sourced Vitis library written in C++ for accelerating Motor Control applications in a variety of use cases.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _brief:

=====================================
Vitis Motor Control Library Tutorial
=====================================

Tutorial Overview
----------------------------------------------

Motor Control Library provides four algorithm-level synthesizable APIs including FOC, SVPWM_DUTY, PWM_GEN and QEI. And a simple virtual motor model is provided for verification.

Based on the Motor Control library with a virtual motor model, you can complete all core module verifications such as FOC, QEI, and SVPWM solely in the Vitis environment. This might be helpful to improve the efficiency of design modification and iterations. This tutorial is designed to guide users on how to conduct design verification and learn how to validate design functionality at different stages of the design process.

Lab-1: Downloading the Library and Understanding the structure
---------------------------------------------------------------------------------------------

Download the Vitis Motor Control Library

.. code-block:: shell

   #!/bin/bash
   git clone https://github.com/Xilinx/Vitis_Libraries.git
   cd Vitis_Libraries/motor_control
   git checkout next

Get knowledge about directory structure of Vitis Motor Control Library

.. code-block:: shell

   ├── docs------------------------document
   │   └── src
   │       ├── benchmark
   │       ├── guide_L1
   │       │   └── IPs
   │       └── images
   └── L1
      ├── include
      │   └── hw------------------L1 APIs
      ├── meta
      └── tests-------------------L1 Test
         ├── IP_FOC
         │   └── src
         ├── IP_PWM_GEN
         ├── IP_QEI
         │   └── src
         ├── IP_SVPWM
         │   └── src
         └── Model---------------model files include virtual motor model

Get knowledge about the virtual motor model

A simplified physical motor model can be represented as follows:

.. image:: /images/tutorial_motor1.png
   :alt: motor1
   :scale: 70%
   :align: center

.. image:: /images/tutorial_motor2.png
   :alt: motor2
   :scale: 70%
   :align: center

The corresponding matrix representation is shown below:

.. image:: /images/tutorial_equation.png
   :alt: equation
   :scale: 70%
   :align: center

The following datasheet shows parameters used in the motor model.

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

These parameters could be set in the header file common.hpp in ./motor_control/L1/include/hw/ folder, the virtual motor model could be found in ./motor_control/L1/tests/Model/model_motor.hpp.

Lab-2: Simulation and verification of SVPWM-DUTY
------------------------------------------------

CSIM verification flow

   .. code-block:: shell

      source <Vitis_install_path>/Vitis/2023.2/settings64.sh
      git clone https://github.com/Xilinx/Vitis_Libraries.git
      cd motor_control/
      git checkout next 
      git pull
      cd L1/tests/IP_SVPWM/
      make run CSIM=1

Execute the executable file with parameters. Follow the step below:

   .. code-block:: shell

      cd ./motor_control/L1/tests/IP_SVPWM_DUTY/svpwm_duty_sim.prj/sol1/csim/build
      ./csim.exe condition: [-shift_0/-shift_120] | [-dc_adc/-dc_ref] | [-pwm_fq <pwm frequency>] | [-dead <dead cycles>] [-ii <sampling II>] [-cnt <Number of input>]   

Stdout explanation

.. image:: /images/tutorial_svpwm_duty_stdout.png
   :alt: svpwm_duty_stdout
   :scale: 70%
   :align: center

The stdout has shown the AXI-lite signal and inner status monitoring parameters.

* ``stt_cnt_iter``: is used to monitor the inner trip count of svpwm_duty.  It can register how many inputs are consumed totally. The default value is 10.
* ``args_dc_link_ref``: is supposed to connect with the external direct-current voltage source for reference. Typically, if the magnitude of the synthesized electric vector is smaller than the value, the high voltage duration of the switch pair should work longer. The duration should multiply a compensate-ratio factor.
* ``args_dc_src_mode``: decides where the external source of dc_link voltage for reference is coming from. 0, the dc_link reference voltage is determined by the measured value of the ADC dc_link voltage; 1, the reference voltage is solely determined by the preset static register value for reference voltage.
* ``args_sample_ii``: it can control the consumption rate of SVPWM_DUTY. The svpwm is producing the waveform every cycle. The default sample-ii value is 1. 
   1. In the CSIM, the fifo is assumed to be infinitely large. Thus, all of the 10 inputs are able to be stacked into the fifo and produce exactly the same number of outputs. Thus, no matter of what sample-ii value is here, the CSIM result always remains same here.
   2. In the COSIM, since we define the fifo depth as four, the fifo can only take in the maximum number of four inputs. If the sample-ii is 1 clock cycle, while the downstream of PWM_GEN is consuming the data every 1000 cycles, the input fifo is soon stuffed full of four inputs and other six inputs are discarded immediately. However, if the sample-ii is set as 1000 clock cycles, every data consumes at a rate of 1000. It can guarantee that no data is going to be discarded. The COSIM behavior complies with the CSIM.
* ``dc_link_adc``: is the measured value of ADC dc_link value.
* ``V_ref``: is rounded value of dc_link_adc
* ``ratio_compensate``: multiply with the duty_cycle in each channel to compensate the final synthesized voltage.

Waveform explanation and check

1. vim ./motor_control/L1/tests/IP_SVPWM_DUTY/svpwm_duty_sim.prj/sol1/csim/build/wave_all10
2. copy the content to excel and draw the curve

Phase_shift_mode = MODE_PWM_PHASE_SHIFT::SHIFT_ZERO

.. image:: /images/tutorial_svpwm_duty_csim_shiftzero.png
   :alt: svpwm_duty_csim_shiftzero
   :scale: 70%
   :align: center

Phase_shift_mode = MODE_PWM_PHASE_SHIFT::SHIFT_120

.. image:: /images/tutorial_svpwm_duty_csim_shift120.png
   :alt: svpwm_duty_csim_shift120
   :scale: 70%
   :align: center

COSIM verification flow

   .. code-block:: shell

      make run COSIM=1 XPART=xc7z020-clg400-1

Waveform explanation

Verify latency from input to output.

.. image:: /images/tutorial_svpwm_duty_cosim1.png
   :alt: svpwm_duty_cosim1
   :scale: 70%
   :align: center

According to waveform, the measured latency is 504.750ns from sampling commands to outputting ratios (50 cycles).

Verify latency of pwm_cycle 

.. image:: /images/tutorial_svpwm_duty_cosim2.png
   :alt: svpwm_duty_cosim2
   :scale: 70%
   :align: center

II-sample's impact

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

II-sample = 1

The fifo depth is 4 and each iteration latency is 93 clock cycles.  Each output is programmed to come out at an interval of 1000 clock cycles. f the sample-ii is 1, the fifo is soon filled full of the data (within 93 clock cycles) while the first output is still not coming yet.  Another 6 data are immediately discarded. The following diagram depicts that the final outcome only has four duty_cycles.

.. image:: /images/tutorial_svpwm_duty_cosim_iisample1.png
   :alt: svpwm_duty_cosim_iisample1
   :scale: 70%
   :align: center

II-sample = 50

The fifo depth is 4 and each iteration latency is 93 clock cycles. When ii-sample is 50, one signal is consumed and the result comes after 93. The fifo has enough space to store the next two stream-in data. Hence, the output end can produce the complete ten waveform.

.. image:: /images/tutorial_svpwm_duty_cosim_iisample50.png
   :alt: svpwm_duty_cosim_iisample50
   :scale: 70%
   :align: center

II-sample = 1000 (Default)

.. image:: /images/tutorial_svpwm_duty_cosim_iisample1000.png
   :alt: svpwm_duty_cosim_iisample1000
   :scale: 70%
   :align: center

Export IP flow

   .. code-block:: shell

      source <Vitis_install_path>/Vitis/2022.2/settings64.sh
      git clone https://github.com/Xilinx/Vitis_Libraries.git
      cd motor_control/
      git checkout next 
      git pull
      cd L1/tests/FOC/IP_SVPWM/
      make run VIVADO_IMPL=1  XPART=xc7z020-clg400-1

You can get an AXI address by using the following command.

   .. code-block:: shell

      vim ./motor_control/L1/tests/IP_SVPWM/svpwm_hls_lib.prj/sol1/impl/verilog/hls_svpwm_duty_pwm_args_s_axi.v

Lab-3: Simulation and verification of PWM-GEN
---------------------------------------------

CSIM verification flow

   .. code-block:: shell

      source <Vitis_install_path>/Vitis/2023.2/settings64.sh
      git clone https://github.com/Xilinx/Vitis_Libraries.git
      cd ./motor_control/
      git checkout next 
      git pull
      cd L1/tests/IP_PWM_GEN/
      make run CSIM=1

Stdout explanation

.. image:: /images/tutorial_pwm_gen_stdout.png
   :alt: pwm_gen_stdout
   :scale: 70%
   :align: center

The stdout shown above depicts the following info:

* ``stt_pwm_cycle``: the pwm_cycle is defined as the ``clock_freq / args_pwm_freq``, which is supposed to have a default value of 1000. Typically, it can help to test and verify the RTL behavior.
* ``args_pwm_freq``: the pwm frequency is the wave frequency after the pulse width modulation. Within every pwm cycle = ``[1 / args_pwm_freq]``, the high voltage duration can be modulated as any value within the range of 0~pwm_cycle.
* ``args_dead_cycles``: the upper and lower switches on the same bridge cannot switch simultaneously, otherwise the overloaded transient current on this branch causes irrevertible damage to the system.  Thus, for every complementary swtich pair, the upper switch reacts a dead_cycles times ahead of the lower switch.
* ``args_phase_shift``: it determines the phase_shift mode. 0, there is no phase_shift in the gating and sync. 1, there is 120-degree phase_shift in the svpwm.
* ``args_sample_ii``: it determines the consumption rate of hls_pwm_gen. The default consumption rate is ii = 1000. 
   1. In the CSIM, the fifo is assumed to be infinitely large. Thus, all of the 10 inputs are able to be stacked into the fifo and produce exactly the same number of outputs. Thus, no matter of what sample-ii value is here, the CSIM result always remains same here.
   2. In the COSIM, the default fifo depth is 2.  The hls_pwm_gen consumes every input and produces 1000 output, with a pipeline interval of 1 clock cycle. Hence, ii > 1000 has the previous input produce the output twice.  The COSIM -ii=2000 diagram depicts the phenomenon.

COSIM verification flow

Waveform explanation, in which II = 1000 complete 10 waveform.

.. image:: /images/tutorial_pwm_gen_cosim1.png
   :alt: pwm_gen_cosim1
   :scale: 70%
   :align: center

-ii = 2000

The hls_pwm_gen read once at the beginning, so produce the exact waveform as [-ii = 1000]. However, since the sampling rate is set as [-ii = 2000], the wave produces as the usual rate., the wave after 1 is producing the wave twice.

.. image:: /images/tutorial_pwm_gen_cosim2.png
   :alt: pwm_gen_cosim2
   :scale: 70%
   :align: center

Export IP flow

   .. code-block:: shell

      source <Vitis_install_path>/Vitis/2022.2/settings64.sh
      git clone https://github.com/Xilinx/Vitis_Libraries.git
      cd ./motor_control/
      git checkout next 
      git pull
      cd L1/tests/IP_PWM_GEN/
      make run VIVADO_IMPL=1  XPART=xc7z020-clg400-1

User can get AXI address by using the command below.

   .. code-block:: shell

      vim ./motor_control/L1/tests/IP_PWM_GEN/pwm_gen.prj/sol1/impl/verilog/hls_pwm_gen_pwm_args_s_axi.v


Tutorial Summary
------------------

Through the above tutorial labs, it can be seen that the development process based on Vitis HLS can cover most of the development and validation work in traditional RTL based flow and C/C++ based flow, and the efficiency is very high. When users need customized changes and optimized designs, they can benefit from Vitis' development flow.

The tutorial will be developed to cover more Motor Contorl motheds and their combinations, flows, and classic applications.

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim: