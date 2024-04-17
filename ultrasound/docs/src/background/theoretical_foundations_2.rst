.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

Theoretical foundations 2
=========================

.. toctree::
   :hidden:
   :maxdepth: 1

Focusing Theory
---------------

Introduction
------------

Before starting with the formulation of focusing and virtual sources for SA and PW, it is essential to introduce the coordinate system for the ultrasound probe, as shown in the following figure:

.. <div style="width: 100%;">
..     <img src="images/probe-coord.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="Probe Coordinates">
..     <p style="text-align: center; font-style: italic">Fig. 5 - Probe coordinate system</p>
.. </div>

It is essential to underline the fact that the direction of propagation of the waves is on the Z axis, which later leads to a significant optimization in terms of computational resources.
Moreover, this system implies that the transducers are represented symmetrically, and they are split in two different and equivalent groups, one with X coordinate negative and the other with X coordinate positive depending on the values of Kerf and dimension of the single element.

Design of virtual transmit sources

Whether you use SA or PW, which could be re-written one as the other, it is important to use virtual transmit sources.
Virtual transmit sources are a construct that overcomes the problem of increasing the SNR parameter in SA while increasing the investigation depth. The calculation of the transmitted field is ambiguous if multiple transducers are involved. The emit focal point is then considered as a source of spherical emission itself, and then the computation of the propagation in transmission became uniquely computable by the formula:

.. <div style="width: 100%;">
..     <img src="images/virtual-sources-formula.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="Virtual Sources formula in transmission">
..     <p style="text-align: center; font-style: italic">Fig. 6 - Virtual Transmit Source Transmission (SA)</p>
.. </div>

*rf* is the position of the virtual source, *tf* is the time taken, including delays, to propagate to the virtual source. The reception is time has a simpler formula (assuming *c* 1540 m/s, speed of sound while propagating in the tissues):

.. <div style="width: 100%;">
..     <img src="images/virtual-sources-rx.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="Virtual Sources formula in reception">
..     <p style="text-align: center; font-style: italic">Fig. 7 - Rx time computation</p>
.. </div>

Where *ri* is the position of the emitter (on the probe) and *rp* is the position of the point that into which the focusing has been calculated.

Knowing the formulation of virtual sources, the SA and PW formulation follows easily.

Synthetic Aperture Formulation
------------------------------

The formulation of the SA is shown below:

.. <div style="width: 100%;">
..     <img src="images/virtual-sources-sa-formula.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="Virtual Sources formula SA">
..     <p style="text-align: center; font-style: italic">Fig. 8 - SA formula computation</p>
.. </div>

The time computation consists in the simple summation of the transmission and reception time.
For sake of clarity, it is also left an example of the system with respect to the coordinates of the ultrasound probe:

.. <div style="width: 100%;">
..     <img src="images/LP-SA-virtual-sources.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="Virtual Sources formula SA">
..     <p style="text-align: center; font-style: italic">Fig. 9 - Formulation for SA</p>
.. </div>


Plane Wave Formulation
----------------------

For PW, the reception formula is the same. However, the transmission is computed in a different manner and is formulated as follows:

.. <div style="width: 100%;">
..     <img src="images/virtual-sources-pw-transmission.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="Virtual Sources formula SA">
..     <p style="text-align: center; font-style: italic">Fig. 10 - Formulation for PW in transission</p>
.. </div>

The formula however is still computed as for SA, with the summation of time in transmission and time in reception.
For sake of clarity, as for SA, it is also left as an example of the system with respect to the coordinates of the ultrasound probe:

.. <div style="width: 100%;">
..     <img src="images/virtual-sources-PW-system.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="Virtual Sources formula SA">
..     <p style="text-align: center; font-style: italic">Fig. 10 - Formulation for PW in transission</p>
.. </div>

Final remarks
-------------

The formulation of virtual sources are no less important than other parts in the Beamforming formulation, as they are fundamental to compute a valid delay time and select the correct samples for the interpolation. Not all the samples are valid, and so we must interpolate just between the indexes of samples that are considered valid. 
This process is fundamental to increase the investigation depth and a high signal quality due to to the higher SNR generated.
*One third of the beamforming process consists in the selection of the samples, and so it has been important to detail the aspects that regards this process.*