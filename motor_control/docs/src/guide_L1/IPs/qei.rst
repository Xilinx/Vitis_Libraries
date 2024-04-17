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


************************************************
Internal Design of Quadrature Encoder Interface 
************************************************

Overview
===========

Quadrature Encoder Interface(QEI) module provides the interface to incremental encoders for obtaining mechanical position data. The quadrature encoder detect position and speed of rotating motion systems. And it has three output channels, channel A, channel B, and index I, provides information on the movement of the motor shaft, including distance and direction. Channel A and B are coded ninety electrical degrees out of phase, and the controller can determine direction of movement based on the phase relationship between Channels A and B. 

Implemention
============

In the implemenation, you act on the counter at every transition of either A or B, thus following X4 encoding. And you should choose the encoder type of direction for Channel A leading B or B leading A. The detail algorithm implementation is illustrated as below:

.. image:: /images/qei_design.png
   :alt: desigh of qei 
   :width: 70%
   :align: center

As it is shown in the aboved pictures, the whole QEI has three functions and dataflow between these functions.

* filterIn: This module uses digital noise filters to reject noise on the incoming quadrature phase signals and index pulse. The input signal needs "max_filtercount" consecutive cycles to output valid signal. The variable of "max_filtercount" can be initialized to 16, or other values.

* catchingEdge: This module captures valid signal by catching on the rising and falling edges of the pulse train. You can mark the state and type of the pulse and filter out invalid signals.

* calcCounter: This module calculates edges feature by counting the leading and trailing edges, the counter monitors the transition in its relationship to the state of the opposite channel, and can generate reliable position and speed. You can initialize the variable mode, and it represents encoding mode, B leading A is represented by 0 and A leading B is represented by 1. The direction of system, clockwise is represented by 1 and counterclockwise is represented by 0 both two encoding mode.

Profiling 
============

The Post-Synthesis Resource usage is shown in the following table.
The QEI C/RTL co-simulation on CPU, and the result is based in table2.   

.. table:: Table 1 Post-Synthesis Resource usage
    :align: center

    +------------+-----------+-----------+----------+----------+--------+
    |    Name    |    LUT    |  Register |   BRAM   |   URAM   |   DSP  |
    +------------+-----------+-----------+----------+----------+--------+
    |     QEI    |    867    |    976    |     0    |    0     |    5   |
    +------------+-----------+-----------+----------+----------+--------+


.. table:: Table 2 Case for QEI Cosim time
    :align: center

    +-------------+----------+-----------+-----------+------------+-------------+
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
    | 1. Time unit: ms.

.. toctree::
    :maxdepth: 1
