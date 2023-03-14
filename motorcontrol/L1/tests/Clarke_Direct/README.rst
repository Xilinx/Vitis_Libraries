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

======================================================================
Xilinx Clarke Direct transform Modulation
======================================================================
The design contain hardware accelerated implementation of Clarke Direct transform 
algorithm. The Clarke Direct transform algorithm is used to converts 
balanced three-phase quantities into balanced two-phase quadrature quantities. 

**Description:** Test Design to validate Clarke Direct Algorithm.

**Top Function:** Clarke_Direct_axi

Results
-------

================ ======= ========= ======= ====== ====== 
Module             DSP     LUT       FF     BRAM   URAM 
SVPWM               1      115       116      0      0 
================ ======= ========= ======= ====== ======
