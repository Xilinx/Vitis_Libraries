..
   Copyright 2022 Xilinx, Inc.

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

Vitis Ultrasound library provides implementation of different L1/L2/L3 APIs Toolbox for ultrasound image processing.

This lib created with the purposes of documenting and explaining how to use Ultrasound Beamforming APIs as a Toolbox based on AIE design. The Library contains all the function and some examples on how to build a custom Beamformer.

Current version provides:

- L1, the lowest level of abstraction and is composed of simple BLAS operation
- L2, the functional units of the Beamformer, which can be obtained by composing L1 libraries
- L3, the example for connected units for 3 beamformer of PW/SA/Scanline

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


