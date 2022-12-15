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

Apodization
-----------

Apodization is a digital signal processing (DSP) operation which is used when windowing temporal signal for different operations, the most famous one is the Short Time Fourier Transform. There are various apodization schemes, spanning from simple box functions to more complex structures. The apodization function must be used and constructed very carefully. For example, using a box function in DSP would probably cause lateral lobes as the transformation in the frequency domain creates a sinc function.

Interpolation in Ultrasound
---------------------------

Interpolation is used because of the virtual sources, if the sampling density which returns us the values is not high enough, it generates significant sidelobes in the point of spread, which results in a significant downgrade of the final image resolution.

This process cannot be overcome in any way (except by raising sampling density, which may not be possible) unless you use interpolation. This construct allows you to generate the intermediate missing points from the samples without increasing the sampling density. The interpolation which can be used in ultrasound application is an enormous variety with different outcomes with respect to the techniques chosen. The most popularly studied interpolation schemes can be grouped under five categories:

Nearest Neighbor (as shown in the application of apodization on valid samples selected)
1.Linear
2.Spline
3.Matched Filter
4.Polynomial

Definitely, the best choices (but at the cost of a high number of computational resources) are ‘Matched Filter’ and ‘Spline’. Linear Interpolation suffers from high sidelobes energy, resulting in a poor final contrast (which is essential for a good quality image in ultrasound beamforming). Polynomial interpolation (especially for high rank ones) suffers from Runge's phenomena and thus it might yield some singular points. Before providing details on the interpolation method chosen (the Spline).