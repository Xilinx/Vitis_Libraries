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

Theory of apodization
---------------------

Introduction

Apodization is a digital signal process (DSP) operation which is used when windowing temporal signal for different operations, which probably the most famous involved one is the *Short Time Fourier Transform*. There are various apodization schemes, spanning from simple box functions to more complex structures. The apodization function must be used and construct very carefully, as for example, using a box function in DSP would probably cause lateral lobes as the transformation in the frequency domain creates a sinc function.

Formulation in the library

In the implementation of the library has been used a dynamic Hanning Window, with the following formula:

.. <div style="width: 100%;">
..     <img src="images/hanning-window-formulation.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="Hanning Window">
..     <p style="text-align: center; font-style: italic">Fig. 11 - Formulation of the Hanning Window</p>
.. </div>

All the values of the absolute value of *x* which are greater than 1 are discarded in this formulation.
The question now is how *x* is formulated? To answer the question we give an example formulation for a linear transducer (as for previous focusing, works also with phased array or other curvilinear probes):

..  <div style="width: 100%;">
..     <img src="images/apodization-formulation.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="apodization formulation">
..     <p style="text-align: center; font-style: italic">Fig. 12 - Formulation with respect to probe</p>
.. </div>

*D* is the point at a certain depth of investigation, which can be easily be extracted by the geometric formula:

..  <div style="width: 100%;">
..     <img src="images/apo-distance-formulation.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="apodization formulation">
..     <p style="text-align: center; font-style: italic">Fig. 13 - Depth formulation</p>
.. </div>

It is important to underline that all the distances are calculated in this case always with respect to the relative transducers positions with respect to the investigation depth,and thus the formulation of *X*, knowing *D* follows with respect to the transducers (which position is know a priori in our system) we are calculating the apodization:

..  <div style="width: 100%;">
..     <img src="images/Xi-apo-formulation.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="apodization formulation">
..     <p style="text-align: center; font-style: italic">Fig. 14 - Vector of values for apodization</p>
.. </div>

Taking a look at the formulation of *X* it follows also why the *F number* (F#) is really important when performing Beamforming. The *F number* actually determines the aperture of the acceptance curve in the application and thus influence greatly the samples in the window for the transducer we are analyzing. Generally, the accepted *F number* is between 0.5 and 2, however, the supposed *F number* to be used using this library is 0.5.

Theory of Interpolation
-----------------------

Introduction

Interpolation is an operation which is used to create new data points given a discrete set. Those new points should represent a curve, or a set of curves, which binds all the given discrete points altogether (in case of upsampling). Following an example:

..  <div style="width: 100%;">
..     <img src="images/bspline.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="example of interpolation">
..     <p style="text-align: center; font-style: italic">Fig. 15 - interpolation</p>
.. </div>

In red we have the fixed point given and in blue the line which consists in the new point created by the interpolation operation. It is going to be detailed it better later, but the image example is what you actually will find implemented in the library, a B-Spline constructed from a variant of the Splines called Catmull-Rom.

Interpolation in Ultrasound

Interpolation is used because of the virtual sources, if the sampling density which returns us the values is not high enough, generates a significant sidelobes in the point of spread, which result in a significant downgrade of the final image resolution. This process cannot be overcomed in any way (excepting from rising the sampling density, which is not necessarily possible) but to the use of interpolation. This construct in fact allows to generate the intermediate missing points from the samples without increasing the sampling density.
The interpolation which can be used in ultrasound application is an enormous variety with different outcomes with respect to the technique chosen. 
The most popular studied interpolation scheme could be regroup in 5 categories:

1. Nearest Neighbour (which consists in the application of the apodization on the valid samples selected);
2. Linear;
3. Spline;
4. Matched Filter; 
5. Polynomial;

Definitely the best choices (but at the cost of a high number of computational resources) are Matched Filter and Spline. Linear Interpolation suffers from high sidelobes energy, which results in a poor final contrast (which is essential for a good quality image in ultrasound beamforming). Polynomial interpolation (especially for high rank ones) suffers from Runge's phenomena and thus it might yield some singular points.
Before entering the details of the interpolation chosen, the spline, we detail why it is a suitable choice for a generic interpolation application.

Spline Interpolation

A bit of hystory may give an understanding of the validity of splines. Before computers were used for creating engineering designs, designer employed drafting tools and drawing by hand. To draw curves, especially for shipbuilding, draftsmen often used long, thin, flexible strips of wood, plastic, or metal called "Splines". The splines were held in place with lead weights. The elasticity of the spline material combined with the constraint of the control points, or **knots**, would cause the strip to take the shape that minimized the energy required for bending it between the fixed points, this being the smoothest possible shape.

To visualise why the Splines are better than another type of interpolation let us compare the previous image obtained with a B-Spline with the one we would end up using linear interpolation:

..  <div style="width: 100%;">
..     <img src="images/linear.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="example of linear interpolation">
..     <p style="text-align: center; font-style: italic">Fig. 16 - linear interpolation</p>
.. </div>

What we look for, when we interpolate in this type of application, is a smooth curve which connects the points with the minimum curvature possible. If we compare Linear interpolation with the previous B-Spline it is immediately evident how the Linear interpolation does not provide any curvature and thus the result of the process is just a strict connection between the point which is not what we are aiming for.
As for the physical Spline the interpolator Spline, has the property of interpolating two points between each other with a curve that has the least curvature possible. For this peculiar property, the Spline has been chosen among the other possible interpolation scheme.

Implemented Spline

In the Ultrasound library, it is proposed as an interpolation scheme a variant of Catmull-Rom Spline where the knot parameter (alpha in figure 19) is fixed a priori to 0 to avoid the re-computation of the interpolant vector every time a set of four points is passed to the function.

How does Catmull-Rom Spline works? Catmull-Rom splines works by selecting 4 points and find the best point between the median point of the four.
Given the following 4 points:

.. <div style="width: 100%;">
..     <img src="images/cr-points.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="example of points">
..     <p style="text-align: center; font-style: italic">Fig. 17 - Example of 4 random points</p>
.. </div>

The result of the interpolation between the median points is the following:

.. <div style="width: 100%;">
..     <img src="images/four-point-cr.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="example of points interpolated by CR">
..     <p style="text-align: center; font-style: italic">Fig. 18 - Interpolation of 4 random points</p>
.. </div>

As it is easy to see the extreme points are used in the interpolation but not interpolated, and thus to include them in the interpolation process the initial and final point of rf-dataset are augmented with padding. 

The formulation of the Catmull-Rom used is the one provided by Barry and Goldman, explained in the following two images:

.. <div style="width: 100%;">
..     <img src="images/CR-formula.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="GB formulaiton of CR">
..     <p style="text-align: center; font-style: italic">Fig. 19 - Catmull-Rom formulation from Goldman and Barry - 1</p>
.. </div>

.. <div style="width: 100%;">
..     <img src="images/Cubic_Catmull-Rom_formulation.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="GB formulaiton of CR">
..     <p style="text-align: center; font-style: italic">Fig. 20 - Catmull-Rom formulation from Goldman and Barry - 2</p>
.. </div>