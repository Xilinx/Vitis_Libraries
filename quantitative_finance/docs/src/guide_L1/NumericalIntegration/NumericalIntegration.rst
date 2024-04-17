

.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: fintech, trapezoidal, Simpson, Romberg
   :description: Three Numerical Integration methods are included: the Adaptive Trapezoidal method, the Adaptive Simpson method and the Romberg method.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


*****************************
Numerical Integration Methods
*****************************

Overview
========

Three Numerical Integration methods are included: the Adaptive Trapezoidal method, the Adaptive Simpson method, and the Romberg method. 

Adaptive Trapezoidal Theory
===========================

The trapezoidal rule works by splitting the function to be integrated up into a number of equal chunks and for each chunk, the curve is approximated by a straight line. In effect, the curve is approximated by a number of trapezoids. The area under the curve then can be approximated by summing the area of all the trapezoids. See `wiki Trapezoidal entry`_ for the theory.

.. _wiki Trapezoidal entry: https://en.wikipedia.org/wiki/Trapezoidal_rule

The Adaptive Trapezoidal rule takes an advantage of the fact that a curve, or part of a curve, that is fairly straight (a small second derivative) needs far fewer chunks than a rapidly changing part of the curve. This rule chunks up the curve in a variable way only using more chunks where they are absolutely required. The algorithm uses a required tolerance to determine how many chunks a given part of the curve requires.

Adaptive Simpson Theory
=======================

The Simpson rule works in a very similar way to the Trapezoidal. However, rather than approximating the curve with a series of straight lines, it approximated the curves with a number of quadratic curves. See `wiki Simpson entry`_ and `wiki Adaptive Simpson entry`_ for the theory.

Romberg Theory
==============

The Romberg method uses a combination of the Trapezoidal Rule and Richardson's Extrapolation to approximate the integral. Richardson's Extrapolation uses a weighted combination of two estimates to generate a more accurate third estimate. See `wiki Romberg entry`_ for the theory.

.. _wiki Trapezoidal entry: https://en.wikipedia.org/wiki/Trapezoidal_rule
.. _wiki Simpson entry: https://en.wikipedia.org/wiki/Simpson%27s_rule
.. _wiki Adaptive Simpson entry: https://en.wikipedia.org/wiki/Adaptive_Simpson%27s_method
.. _wiki Romberg entry: https://en.wikipedia.org/wiki/Romberg%27s_method

