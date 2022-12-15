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

Theoretical foundations 3
=========================

.. toctree::
   :hidden:
   :maxdepth: 1

The six parts of the beamforming
--------------------------------

Our beamformer is a Sum-And-Delay (SAD) one, and thus allow the user to select which level of parallelism the user would like to work on. In the example of design (UltraFast Imaging - Plane Wave) which can be found in this lounge, the level of parallelism is set on the emission level, and thus we can have up to 8 instances of Beamforming which computes 16 lines each. It is suggested however to take advantage of the enormous I/O performances of the Versal board and try to place more beamformers which computes less lines per instance (due to a limitation of the memory SRAM available in the PL), We will come back later on this.

The beamforming can then be divided into 6 categories labelled as:

1.Image Points
2.Focusing
3.Delay
4.Samples Selection
5.Apodization
6.Interpolation

Image Points
------------

This is the first part of the algorithm; it computes the values of the points (virtual sources) that form a line of the image and returns a Nx3 matrix with the ordered points. The three dimensions represent our Euclidean space, as our beamformer is constructed to compute 3D values of space. This first part of the algorithm depends heavily on the starting point of investigation, investigation depth, the sampling density and thus the step between virtual sources we are keen to accept. The bigger the matrix of points which represent a line of our image in fact is, the higher the number of operations are going to be required from the following steps except for focusing.

Focusing
--------

Please note that this function is only needed when working with dynamic apodization.

Find the distance between the probe and the point we are using as a reference for the apodization. The reference apodization can be computed as the starting point of the investigation with the Z component nullified. This function is an absolute value which is transducer specific and is computed every line for the whole transducers.

Delay
-----

This part consists in computing the transmit delay for a line of the image. The values keep in consideration the relative position of the virtual sources. This part of Beamformer is dependent on the computation of the Image Points (which returns the value for virtual sources), the direction of propagation of the system and the reference point of the transmitter. This last value depends on the span of emission of the Plane Waves created or on the transmit focus for Synthetic Aperture (called delay distance in the beamformer).

Samples Selection
-----------------

This part is the computation of the receiving delay to which we add the result of delay to obtain the full computation of the time delay. This operation depends on the result of both Delay and Image Points. The other parameters used are the positions of the transducers (same as for Focusing), the velocity of propagation of sound in the body and the sampling frequency selected of the probe.

Apodization
-----------

Please note that this function is only needed when working with dynamic apodization.

This function computes a dynamic symmetric Hanning window which returns for every virtual source whether or not the values are considered in the aperture or not. In the case of Plane Wave, these values are computed for reception for every transducer, whereas for Synthetic Aperture they are computed once per line for the transmitter and for every transducer in reception. The function is dependent on the result of Focusing and Image Points. The other values on which the computation of the dynamic apodization is dependent on are the F number, the direction of propagation and the apodization reference (same as per Delay).

Interpolation
-------------

This is the last part of the Beamforming and is the responsible to perform Spline Interpolation on the valid samples. This last part depends on Samples for the selection of the valid samples to be interpolated.