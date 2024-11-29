.. 
   .. Copyright © 2019–2024 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _release_note:

Release Note
============

.. toctree::
   :hidden:
   :maxdepth: 1

.. note:: Known Issue

    * L3/tests/scanline_AllinAIE interrupted issue with sw_emu target. This will get fixed in the next release.

AMD Vitis |trade| Ultrasound library provides not only toolbox-style implementation of different L1/L2/L3 APIs for ultrasound image processing, but also an all-in-AIE implementation of scanline imaging algorithm. You can directly use the function combinations based on toolbox APIs to customize algorithms, and can also start from or refer to the all-in-AIE scanline implementation to obtain the final design.

2022.2
-------
- L1, the lowest level of abstraction and is composed of simple BLAS operation
- L2, the functional units of the Beamformer, which can be obtained by composing L1 libraries
- L3, the example for connected units for 3 beamformer of PW/SA/Scanline

2023.1
-------
- L1, imgrate window port to buffer port for aligning latest aie compiler feature
- L2, refine graph for dimension definition, now it could be inferred from template configuration
- L3, refine L3 graph for including L2 graphs as sub-graph and set the L3 beamformer as top graph

2023.2
-------
- L1, contains not only the fine-grained arithmetic kernels, but also the coarse-grained algorithm kernels of scanline. The algorithm kernels have interfaces for both data units and run-time parameters, making it easier to extend functionalities
- L2, provides two kinds of sub-graphs for sub-algorithm modules and a scanline L2 top-graph. One kind of sub-graph is directly based on L1 arithmetic kernel combination, the other kind is the wrappers of L1 algorithm kernel. The top-graph of scanline can support the end-to-end validation in the L2 level by invoking a C-model of scanline algorithm
- L3, besides the examples for connected units for 3 beamformer of PW/SA/Scanline, L3 provides a end-to-end scanline project based on L2 scanline top-graph which can be verified on VCK190

From 23.2 release, An algorithm-end to AIE-end implementation of scanline are provided in L2 and L3 with a set of scanline C-model functions. These C-model functions show a step-by-step process for how to start from natural description of a complex algorithm to obtain an AIE-mappable implementation. You can also directly invoke the C-models in any level of AIE projects to generate the input and verify the output simultaneously and conveniently. 

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
