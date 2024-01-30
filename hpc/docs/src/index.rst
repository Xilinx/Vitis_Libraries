.. 
   
.. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

=====================
Vitis HPC Library
=====================

AMD Vitis |trade| HPC Library provides an acceleration library for applications with high
computation workload. For example, seismic imaging and inversion, high-precision simulations, 
genomics and so on. Three types of components are provided in this library, 
namely L1 primitives and L2 kernels. These implementations are organized in their
corresponding directories L1 and L2. The L1 primitives' implementations can be leveraged
by FPGA hardware developers. The L2 kernels' implementations provide examples for 
Vitis kernel developers.
This library depends on the **AMD BLAS and SPARSE** library to implement some components.

Because HPC applications normally have high precision requirements, the current supported data
type are mainly single precision floating point type (FP32 type) and double precision floating point type (FP64 type). 
Although, most components can be configured
to support other data types, some of the architectures are specifically optimized to address
FP32 operations. For example,accumulations.

In the current release, three types of applications have been addressed by this library, namely
RTM (Reverse Time Migration), CG (Conjugate Gradient) method, and MLP-based high precession seismic inversion.  
RTM is an important seismic imaging technique used for producing an accurate representation of the subsurface.
The basic computation unit of an RTM application is a stencil module, which is the essential 
step for explicit **FDTD (Finite Difference Time Domain)** solutions. Seismic inversion is a procedure
used to reconstruct subsurface properties via the seismic reflection data. 

Many engineering problems, such as FEM, are eventually transformed to a group of linear systems. 
Conjugate Gradient method, an iterative method, is widely adopted to solve linear systems, 
especially those with highly sparse and large-dimension matrices.
Preconditioner matrix is necessary for most of the problems to achieve convergent results and reduce dramatically the
number of iterations; hence, improves the entire performance. 

Modern technology uses high precision MLP (Multilayer perceptron) based neural network to speed up this process.
The basic unit of a MLP application normally includes a fully connected neural (**FCN**) network and an activation
function. For example, sigmoid function.


In this library, you will find the implementations of stencil module, 2D and 3D RTM forward propagation path,
2D RTM application, CG solvers with Jacobi preconditioner, high-precision fully connected neural network, and sigmoid activation function.


Since all the kernel code is developed with the permissive Apache 2.0 license,
advanced users can easily tailor, optimize, or combine them for their own need.
Demos and usage examples of different implementation level are also provided
for reference. 

.. toctree::
   :caption: Introduction
   :maxdepth: 1

   Overview <overview.rst>
   Release Note <release.rst>
 
.. toctree::
   :caption: User Guide
   :maxdepth: 2 

   Python Environment Setup Guide <pyenvguide.rst>
   L1 Primitives User Guide <user_guide/L1/L1.rst>
   L2 Kernels User Guide <user_guide/L2/L2.rst>

.. toctree::
  :caption: Benchmark 
  :maxdepth: 1 

  Benchmark <benchmark.rst>


Index
-----

* :ref:`genindex` 


.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim: