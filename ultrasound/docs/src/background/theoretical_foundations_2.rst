.. 
   Copyright 2019 Xilinx, Inc.
  
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
  
       http://www.apache.org/licenses/LICENSE-2.0
  
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

Theoretical foundations 2
=========================

.. toctree::
   :hidden:
   :maxdepth: 1

Focusing Theory
---------------

Introduction

Before starting with the formulation of focusing and virtual sources for SA and PW it is essential to introduce the coordinate system for our ultrasound probe, detailed in the following picture:

.. <div style="width: 100%;">
..     <img src="images/probe-coord.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="Probe Coordinates">
..     <p style="text-align: center; font-style: italic">Fig. 5 - Probe coordinate system</p>
.. </div>

It is essential to underline the fact that the direction of propagation of the waves is on the Z axis, which will lead later to a significant optimization in terms of computational resources.
Moreover, this system implies that the transducers are represented symmetrically, and they will be split in two different and equivalent groups, on with X coordinate negative and one with X coordinate positive depending on the values of Kerf and dimension of the single element.

Design of virtual transmit sources

Whether we use SA or PW, which we have seen could be re-written one as the other, there is the necessity to use virtual transmit sources. What are them and why are so useful?
Virtual transmit sources are a construct which overcome the problem of increasing the SNR parameter in SA while increasing the investigation depth. The calculation of the transmitted field is ambiguous if multiple transducers are involved. The emit focal point is then considered as a source of spherical emission itself, and then the computation of the propagation in transmission became uniquely computable by the formula:

.. <div style="width: 100%;">
..     <img src="images/virtual-sources-formula.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="Virtual Sources formula in transmission">
..     <p style="text-align: center; font-style: italic">Fig. 6 - Virtual Transmit Source Transmission (SA)</p>
.. </div>

*rf* is the position of the virtual source, *tf* is the time taken, including delays, to propagate to the virtual source. The reception is time has a simpler formula (assuming *c* 1540 m/s, speed of sound while propagating in the tissues):

.. <div style="width: 100%;">
..     <img src="images/virtual-sources-rx.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="Virtual Sources formula in reception">
..     <p style="text-align: center; font-style: italic">Fig. 7 - Rx time computation</p>
.. </div>

Where *ri* is the position of the emitter (on the probe) and *rp* is the position of the point which the focusing has been calculated into.

Knowing the formulation of virtual sources, the SA and PW formulation follows easily.

Synthetic Aperture Formulation
------------------------------

The formulation of the SA is the following:

.. <div style="width: 100%;">
..     <img src="images/virtual-sources-sa-formula.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="Virtual Sources formula SA">
..     <p style="text-align: center; font-style: italic">Fig. 8 - SA formula computation</p>
.. </div>

The time computation consists in the simple summation of the transmission and reception time.
For sake of clarity, it is left also an example of the system with respect to the coordinates of the ultrasound probe:

.. <div style="width: 100%;">
..     <img src="images/LP-SA-virtual-sources.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="Virtual Sources formula SA">
..     <p style="text-align: center; font-style: italic">Fig. 9 - Formulation for SA</p>
.. </div>


Plane Wave Formulation
----------------------

For PW the reception formula is the same, however the transmission is computed in a different manner and is formulated as follows:

.. <div style="width: 100%;">
..     <img src="images/virtual-sources-pw-transmission.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="Virtual Sources formula SA">
..     <p style="text-align: center; font-style: italic">Fig. 10 - Formulation for PW in transission</p>
.. </div>

The formula however is still computed as for SA, with the summation of time in transmission and time in reception.
For sake of clarity, as for SA, it is left also an example of the system with respect to the coordinates of the ultrasound probe:

.. <div style="width: 100%;">
..     <img src="images/virtual-sources-PW-system.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="Virtual Sources formula SA">
..     <p style="text-align: center; font-style: italic">Fig. 10 - Formulation for PW in transission</p>
.. </div>

Final remarks
-------------

The formulation of virtual sources are no less important than other parts in the Beamforming formulation, as they are fundamental to compute a valid delay time and select the correct samples for the interpolation. Not all the samples are valid, and so we must interpolate just between the indexes of samples that are considered valid. 
Let us recall that this process is fundamental to increase the investigation depth and a high signal quality thanks to the higher SNR generated.
*One third of the beamforming process consists in the selection of the samples, and so it has been important to detail the aspects which regards this process.*