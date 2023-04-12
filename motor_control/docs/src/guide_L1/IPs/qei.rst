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


************************************************
Internal Design of Quadrature Encoder Interface 
************************************************

Overview
===========

Quadrature Encoder Interface(QEI) module provides the interface to incremental encoders for obtaining mechanical position data. The quadrature encoder detect position and speed of rotating motion systems. And it has three output channels, channel A, channel B and index I, provides information on the movement of the motor shaft, including distance and direction. Channel A and B are coded ninety electrical degrees out of phase, and the controller can determine direction of movement based on the phase relationship between Channels A and B. 

Implemention
============

In our implemenation, we are acting on counter at every transition of either A or B, thus following X4 encoding. And we should choose the encoder type of direction for Channel A leading B or B leanding A. The detail algorithm implemention is illustrated as below:

.. image:: /images/qei_design.png
   :alt: desigh of qei 
   :width: 70%
   :align: center

As it is shown in the aboved pictures, the whole QEI have 3 functions and dataflow between these functions.

* filterIn: This module uses digital noise filters to reject noise on the incoming quadrature phase signals and index pulse. The input signal needs "max_filtercount" consecutive cycles to output valid signal. The variable of "max_filtercount" can be initialized to 16, or other values.

* catchingEdge: This module captures valid signal by catching on the rising and falling edges of the pulse train. We can mark the state and type of the pulse and filter out invalid signals.

* calcCounter: This module calculates edges feature by counting the leading and trailing edges, the counter monitors the transition in its relationship to the state of the opposite channel, and can generate reliable position and speed. The user can initialize the variable mode, and it represents encoding mode, B leading A is represented by 0 and A leading B is represented by 1. The direction of system, clockwise is represented by 1 and counterclockwise is represented by 0 both two encoding mode.

Profiling 
============

The Post-Synthesis Resource usage are shown in the table1 below.
The QEI C/RTL co-simulation on CPU, and the result is based it in table2.   

.. table:: Table 1 Post-Synthesis Resource usage
    :align: center

    +------------+-----------+-----------+----------+----------+--------+
    |    Name    |    LUT    |  Register |   BRAM   |   URAM   |   DSP  |
    +------------+-----------+-----------+----------+----------+--------+
    |     QEI    |    867    |    976    |     0    |    0     |    5   |
    +------------+-----------+-----------+----------+----------+--------+


.. table:: Table 2 Case for QEI Cosim time
    :align: center
    
    +------ ------+----------+-----------+-----------+------------+-------------+
    |     CLK     |    CPR   |    RPM    |    DIR    |    angle   |    Cosim    |
    +-------------+----------+-----------+-----------+------------+-------------+
    |    100M     |   1000   |   3000    |     1     |     120    |    459.44   |
    +-------------+----------+-----------+-----------+------------+-------------+
    |    100M     |   1000   |   2000    |     1     |     120    |    449.90   |
    +-------------+----------+-----------+-----------+------------+-------------+
    |    100M     |   1000   |   1000    |     1     |     120    |    475.80   |
    +-------------+----------+-----------+-----------+------------+-------------+
    |    100M     |   1000   |    500    |     0     |     120    |    465.97   |
    +-------------+----------+-----------+-----------+------------+-------------+

.. note::
    | 1. QEI running on platform with Intel(R) Xeon(R) CPU E5-2690 v4 @ 2.60GHz, 28 Threads (14 Core(s)).
    | 2. Time unit: ms.

.. toctree::
    :maxdepth: 1
