..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2023, Advanced Micro Devices, Inc.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

Vitis Ultrasound Library
==========================

Vitis Ultrasound library provides not only toolbox-style implementation of different L1/L2/L3 APIs for ultrasound image processing, but also an all-in-AIE implementation of scanline imaging algorithm. Users can directly use the function combinations based on toolbox APIs to customize algorithms, and can also start from or refer to the all-in-AIE scanline implementation to obtain the final design.

Current version provides:

- L1, contains not only the fine-grained arithmetic kernels, but also the coarse-grained algorithm kernels of scanline. The algorithm kernels have interfaces for both data units and run-time parameters, making it easier to extend functionalities
- L2, provides two kinds of sub-graphs for sub-algorithm modules and a scanline L2 top-graph. One kind of sub-graph is directly based on L1 arithmetic kernel combination, the other kind is the wrappers of L1 algorithm kernel. The top-graph of scanline can support the end-to-end validation in the L2 level by invoking a C-model of scanline algorithm
- L3, besides the examples for connected units for 3 beamformer of PW/SA/Scanline, L3 provides a end-to-end scanline project based on L2 scanline top-graph which can be verified on VCK190


From 23.2 release, An algorithm-end to AIE-end implementation of scanline are provided in L2 and L3 with a set of scanline C-model functions. These C-model functions show a step-by-step process for how to start from natural description of a complex algorithm, to obtain an AIE-mappable implementation. Users can also directly invoke the C-models in any level of AIE projects to generate input and verify output simultaneously and conveniently. 

.. toctree::
   :caption: Introduction
   :maxdepth: 1

   Overview <introduction/overview.rst>
   Release Note <introduction/release.rst>

.. toctree::
   :caption: background
   :maxdepth: 2

   theoretical_foundation <background/theoretical_foundations.rst>
   theoretical_foundation_2 <background/theoretical_foundations_2.rst>
   theoretical_foundation_3 <background/theoretical_foundations_3.rst>

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


