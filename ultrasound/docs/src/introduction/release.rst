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

.. _release_note:

Release Note
============

.. toctree::
   :hidden:
   :maxdepth: 1

Vitis Ultrasound library provides implementation of different L1/L2/L3 APIs Toolbox for ultrasound image processing.

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