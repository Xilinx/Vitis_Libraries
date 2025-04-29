..
   .. Copyright © 2019–2024 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

Vitis Ultrasound Library
==========================

AMD Vitis |trade| Ultrasound library provides not only toolbox-style implementation of various L1/L2/L3 APIs for ultrasound image processing but also an all-in-AIE implementation of scanline imaging algorithm. You can directly use the function combinations based on toolbox APIs to customize algorithms, and you can also start from or refer to the all-in-AIE scanline implementation to obtain the final design.

Current version provides:

- L1, contains not only fine-grained arithmetic kernels, but also coarse-grained algorithm kernels of scanline. The algorithm kernels have interfaces for both data units and run-time parameters, making it easier to extend functionalities.
- L2, provides graph-level APIs including the wrappers of a signle L1 kernel and the compbinations of multiple L1 kernels. The top-graph of scanline can support the end-to-end validation in the L2 level by invoking a C-model of scanline algorithm.
- L3, besides examples for connected units for three beamformers of PW/SA/Scanline, L3 also provides an end-to-end scanline project based on L2 scanline top-graph that can be verified on VCK190.


From 23.2 release, an algorithm-end to AIE-end implementation of scanline is provided in L2 and L3 with a set of scanline C-model functions. These C-model functions show a step-by-step process for how to start from natural description of a complex algorithm to obtain an AIE-mappable implementation. You can also directly invoke the C-models at any level of AIE projects to generate input and verify output simultaneously and conveniently. 

Release 25.1 added support for AIE-ML device(VEK280) with 20 new L1 APIs and 6 new L2 APIs. These APIs accommodate up to three data types (float, int32, and cint16), tailored to the characteristics of specific algorithms.

.. toctree::
   :caption: Introduction
   :maxdepth: 1

   Overview <introduction/overview.rst>
   Release Note <introduction/release.rst>

.. toctree::
   :caption: Features
   :maxdepth: 1

   Features for Ultrasound Library Release <features/features.rst> 
   Details for Ultrasound Library L1 <features/details_L1.rst>
   Details for Ultrasound Library L2 <features/details_L2.rst>
   Details for Ultrasound Library L3 <features/details_L3.rst>

.. toctree::
   :caption: Tutorial
   :maxdepth: 2

   Vitis Ultrasound Library Tutorial <tutorial/tutorial.rst>

.. toctree::
   :caption: Resources
   :maxdepth: 1

   Resources & Documents <resources_documents/resources.rst>

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:

