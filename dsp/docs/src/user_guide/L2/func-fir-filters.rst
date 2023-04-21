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


.. _FILTERS:

=======
Filters
=======


The DSPLib contains several variants of Finite Impulse Response (FIR) filters.
These include single-rate FIRs, half-band interpolation/decimation FIRs, as well as integer and fractional interpolation/decimation FIRs for AIE.
AIE-ML devices have support for only the single rate FIRs. Differences in hardware lead to different constraints and implementations for FIRs in each device.

.. toctree::
   :maxdepth: 1

   FIRs on AIE <func-fir-filtersAIE.rst>
   FIRs on AIE-ML <func-fir-filters-aieml.rst>