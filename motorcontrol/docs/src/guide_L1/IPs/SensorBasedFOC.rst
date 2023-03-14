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


***************************************************************
Internal Design of Sensor based field-orientated control (FOC)
***************************************************************


Overview
==========
The field-orientated control (FOC) system calculates the flux and torque references given by the current from Sensor, then calculates the corresponding voltage component references using the flux and torque. 

As an independent IP, Sensor based FOC could work with SVPWM_DUTY, PWM_GEN and QEI in this library.

The following summarizes three types of FOC based motor control that are to be implemented. Each focuses on either controlling the torque or speed of the motor. 

Torque Control - Default FOC control is focused on maximizing the torque output of a motor by optimizing the quadrature (q) vector which represents the useful motor torque and minimizing the direct (d) vector component. In this mode of operation the goal is to keep motor torque constant by adjusting motor speed.
Speed Control - Speed control is implemented through an additional PI control that adjusts the motor torque to the motor to maintain a constant speed. 
Field Weakening Control - The field weakening control (call "flux control in EDDP) method trades off optimal torque in order to increase the speed of the motor. This is accomplished by adjusting the relationship of the q-vector and d-vector in FOC.

Implemention
==============
The Sensor based FOC Features:

Table 1 : Sensor based FOC Features

.. table:: Table 1 Sensor based FOC Features
    :align: center

    +-----------------------+-----------------------------------------------------------------------+
    | hls_foc_strm_ap_fixed |                               Status                                  |
    +=======================+=======================================================================+
    |       Input           |  3-phase currents non-block synchronous input                         |
    |                       |  Angle and speed synchronous input by a 32bit buffer                  |
    |                       |  RPM: buffer[15:0], revolution per minute                             |
    |                       |  Angle: buffer[32:16], theta_m mormalize to [0, CPR]                  |
    +-----------------------+-----------------------------------------------------------------------+
    |      Output           |  3-phase voltages non-block synchronous output                        |
    +-----------------------+-----------------------------------------------------------------------+  
    |  configurable         |  modes, pid args, manual args...                                      |
    |  &mintorable info     |  pid intermediate results, details check the interface table          |
    +-----------------------+-----------------------------------------------------------------------+
    |    performance        |  Main clk 100Mhz, max throughput 13M/s                                |
    |                       |                                                                       |
    +-----------------------+-----------------------------------------------------------------------+

The algorithm implemention is shown as the figure below:

Figure 1 : Sensor based FOC architecture top view

.. figure:: /images/Design_Sensorfoc.png
      :alt: Figure 1 Sensor based FOC architecture top view
      :width: 60%
      :align: center

There are four modes for users to set normarly:

    * MOD_STOPPED
    * MOD_SPEED_WITH_TORQUE
    * MOD_TORQUE_WITHOUT_SPEED
    * MOD_FLUX

In the Torque control (MOD_TORQUE_WITHOUT_SPEED) implements a closed loop control focused on maintaining a specified torque value. In this mode of operation the q-vector provides the useful torque output of the motor and the d-vector the force that is parallel to the rotor. The d-vector represents non-useful force and thus any non-zero value is considered an error.

Figure 2 : Torque control mode

.. figure:: /images/TorqueWithoutSpeed.png
      :alt: Figure 2 Torque control mode
      :width: 80%
      :align: center

Constant speed control (MOD_TORQUE_WITHOUT_SPEED) is achieved through a PID controller that adjusts the motor torque to maintain a specified motor speed.

Figure 3 : Speed control mode

.. figure:: /images/TorqueWithSpeed.png
      :alt: Figure 3 Speed control mode
      :width: 80%
      :align: center

The field weakening operation of control trades optimal torque for additional speed as the motor reaches its limit at which additional current to the motor does not provide additional torque. 

Figure 4 : field weakening control mode

.. figure:: /images/Fieldweaking.png
      :alt: Figure 4 field weakening control mode
      :width: 80%
      :align: center

Profiling
==========

.. toctree::
   :maxdepth: 1

   benchmark of Sensor Based FOC <../../benchmark/FOC_sensor.rst>
