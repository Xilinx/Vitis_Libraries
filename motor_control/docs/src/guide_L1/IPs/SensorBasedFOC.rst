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


***************************************************************
Internal Design of Sensor based field-orientated control (FOC)
***************************************************************


Overview
==========
The field-orientated control (FOC) system calculates the flux and torque references given by the current from Sensor, then calculates the corresponding voltage component references using the flux and torque. 

As an independent IP, Sensor based FOC could work with SVPWM_DUTY, PWM_GEN, and QEI in this library.

The following summarizes three types of FOC based motor control that are to be implemented. Each focuses on either controlling the torque or speed of the motor. 

Torque Control - Default FOC control is focused on maximizing the torque output of a motor by optimizing the quadrature (q) vector which represents the useful motor torque and minimizing the direct (d) vector component. In this mode of operation, the goal is to keep motor torque constant by adjusting motor speed.
Speed Control - Speed control is implemented through an additional PI control that adjusts the motor torque to the motor to maintain a constant speed. 
Field Weakening Control - The field weakening control (call "flux control in EDDP) method trades off optimal torque to increase the speed of the motor. This is accomplished by adjusting the relationship of the q-vector and d-vector in FOC.

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
    |    performance        |  Main clk 100Mhz, max throughput 20M/s                                |
    |                       |                                                                       |
    +-----------------------+-----------------------------------------------------------------------+

The algorithm implemention is shown as the figure below:

Figure 1 : Sensor based FOC architecture top view

.. figure:: /images/Design_Sensorfoc.png
      :alt: Figure 1 Sensor based FOC architecture top view
      :width: 60%
      :align: center

There are four modes to set normally:

    * MOD_STOPPED
    * MOD_SPEED_WITH_TORQUE
    * MOD_TORQUE_WITHOUT_SPEED
    * MOD_FLUX

In the Torque control, (MOD_TORQUE_WITHOUT_SPEED) implements a closed loop control focused on maintaining a specified torque value. In this mode of operation, the q-vector provides the useful torque output of the motor and the d-vector the force that is parallel to the rotor. The d-vector represents non-useful force and thus any non-zero value is considered an error.

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

The field weakening operation of control trades optimal torque for additional speed as the motor reaches its limit at which an additional current to the motor does not provide additional torque. 

Figure 4 : field weakening control mode

.. figure:: /images/Fieldweaking.png
      :alt: Figure 4 field weakening control mode
      :width: 80%
      :align: center

Profiling
==========

.. toctree::
   :maxdepth: 1

   Benchmark of Sensor Based FOC <../../benchmark/FOC_sensor.rst>
