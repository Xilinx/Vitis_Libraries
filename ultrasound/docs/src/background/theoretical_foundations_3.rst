.. 
  .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

Theoretical foundations 3
=========================

.. toctree::
   :hidden:
   :maxdepth: 1

Theory of apodization
---------------------

Introduction

Apodization is a digital signal process (DSP) operation, which is used when windowing temporal signal for different operations.The most famous one is the *Short Time Fourier Transform*. There are various apodization schemes, spanning from simple box functions to more complex structures. The apodization function must be used and constructed carefully, for example, using a box function in DSP causes lateral lobes as the transformation in the frequency domain creates a sinc function.

Formulation in the library

In the implementation of the library, a dynamic Hanning Window is used with the following formula:

.. <div style="width: 100%;">
..     <img src="images/hanning-window-formulation.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="Hanning Window">
..     <p style="text-align: center; font-style: italic">Fig. 11 - Formulation of the Hanning Window</p>
.. </div>

All the values of the absolute value of *x* that are greater than one are discarded in this formulation.
The example formulation for a linear transducer (as for previous focusing, also works with phased array or other curvilinear probes) explains how an *x* is formulated:

..  <div style="width: 100%;">
..     <img src="images/apodization-formulation.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="apodization formulation">
..     <p style="text-align: center; font-style: italic">Fig. 12 - Formulation with respect to probe</p>
.. </div>

*D* is the point at a certain depth of investigation, which can easily be extracted by the geometric formula:

..  <div style="width: 100%;">
..     <img src="images/apo-distance-formulation.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="apodization formulation">
..     <p style="text-align: center; font-style: italic">Fig. 13 - Depth formulation</p>
.. </div>

All distances are calculated in this case are with respect to the relative transducers positions and the investigation depth,and thus the formulation of *X*, knowing *D* follows with respect to the transducers (which position is know a priori in our system) you are calculating the apodization:

..  <div style="width: 100%;">
..     <img src="images/Xi-apo-formulation.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="apodization formulation">
..     <p style="text-align: center; font-style: italic">Fig. 14 - Vector of values for apodization</p>
.. </div>

When performing Beamforming, the *F number* determines the aperture of the acceptance curve in the application and thus influences greatly the samples in the window for the transducer you analyze. Generally, the accepted *F number* is between 0.5 and 2. However, the supposed *F number* to be used using this library is 0.5.

Theory of Interpolation
-----------------------

Introduction

Interpolation is an operation that is used to create new data points given a discrete set. Those new points should represent a curve, or a set of curves, which binds all the given discrete points altogether (in case of upsampling). Following is an example:

..  <div style="width: 100%;">
..     <img src="images/bspline.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="example of interpolation">
..     <p style="text-align: center; font-style: italic">Fig. 15 - interpolation</p>
.. </div>

In red, you have the fixed point given and in blue, the line, which consists of the new point created by the interpolation operation. The image example is what you find implemented in the library, a B-Spline constructed from a variant of the Splines called Catmull-Rom.

Interpolation in Ultrasound

Interpolation is used because of the virtual sources, if the sampling density that returns the values is not high enough, generates significant sidelobes in the point of spread, which results in a significant downgrade of the final image resolution. This process cannot be overcome in any way (except from rising the sampling density, which is not possible) but to the use of interpolation. This construct in fact allows to generate the intermediate missing points from the samples without increasing the sampling density.
The interpolation which can be used in ultrasound application is an enormous variety with different outcomes with respect to the technique chosen. 
The most popular studied interpolation scheme could be regroup in 5 categories:

1. Nearest Neighbour (which consists of the application of the apodization on the valid samples selected);
2. Linear;
3. Spline;
4. Matched Filter; 
5. Polynomial;

The best choices (but at the cost of a high number of computational resources) are Matched Filter and Spline. Linear Interpolation suffers from high sidelobes energy, which results in a poor final contrast (which is essential for a good quality image in ultrasound beamforming). Polynomial interpolation (especially for high rank ones) suffers from Runge's phenomena and thus it might yield some singular points.
Before entering the details of the interpolation chosen, the spline, understand why it is a suitable choice for a generic interpolation application.

Spline Interpolation

Before computers were used for creating engineering designs, the designer employed drafting tools and drawing by hand. To draw curves, especially for shipbuilding, draftsmen often used long, thin, flexible strips of wood, plastic, or metal called Splines. The splines were held in place with lead weights. The elasticity of the spline material combined with the constraint of the control points, or **knots** causes the strip to take the shape that minimized the energy required for bending it between the fixed points, this being the smoothest possible shape.

To understand why the Splines are better than another type of interpolation, compare the previous image obtained with a B-Spline with the one you have using linear interpolation:

..  <div style="width: 100%;">
..     <img src="images/linear.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="example of linear interpolation">
..     <p style="text-align: center; font-style: italic">Fig. 16 - linear interpolation</p>
.. </div>

When you interpolate in this type of application, there is a smooth curve that connects the points with the minimum curvature possible. If you compare Linear interpolation with the previous B-Spline, it is immediately evident how the Linear interpolation does not provide any curvature and thus the result of the process is just a strict connection between the point, which is not desirable.
As for the physical Spline the interpolator Spline, has the property of interpolating two points between each other with a curve that has the least curvature possible. For this peculiar property, the Spline has been chosen among the other possible interpolation scheme.

Implemented Spline

In the Ultrasound library, it is proposed as an interpolation scheme, a variant of Catmull-Rom Spline where the knot parameter (alpha in figure 19) is fixed a priori to 0 to avoid the re-computation of the interpolate vector every time a set of four points is passed to the function.

Catmull-Rom splines work by selecting four points and find the best point between the median point of the four.
Here are the four points:

.. <div style="width: 100%;">
..     <img src="images/cr-points.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="example of points">
..     <p style="text-align: center; font-style: italic">Fig. 17 - Example of 4 random points</p>
.. </div>

The result of the interpolation between the median points is the following:

.. <div style="width: 100%;">
..     <img src="images/four-point-cr.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="example of points interpolated by CR">
..     <p style="text-align: center; font-style: italic">Fig. 18 - Interpolation of 4 random points</p>
.. </div>

The extreme points are used in the interpolation but not interpolated, and thus to include them in the interpolation process, the initial and final point of rf-dataset are augmented with padding. 

The formulation of the Catmull-Rom used is the one provided by Barry and Goldman, as explained in the following two images:

.. <div style="width: 100%;">
..     <img src="images/CR-formula.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="GB formulaiton of CR">
..     <p style="text-align: center; font-style: italic">Fig. 19 - Catmull-Rom formulation from Goldman and Barry - 1</p>
.. </div>

.. <div style="width: 100%;">
..     <img src="images/Cubic_Catmull-Rom_formulation.png" style="display: block; margin-left: auto; margin-right: auto; width: 70%;" alt="GB formulaiton of CR">
..     <p style="text-align: center; font-style: italic">Fig. 20 - Catmull-Rom formulation from Goldman and Barry - 2</p>
.. </div>