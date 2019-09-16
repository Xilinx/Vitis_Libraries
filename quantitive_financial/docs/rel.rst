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

Release Note
============

Version 0.5
-----------


FinTech library 0.5 provides engines and primitives for the acceleration of quantitative financial applications on FPGA. It comprises two approaches to pricing:

* a family of 10 Monte-Carlo based engines for 6 equity options (including European and American options) using Black-Scholes and Heston models; all of these pricing engines are based on a provided generic Monte Carlo simulation API, and work in parallel due to their streaming nature; and

* a finite-difference PDE solver for the Heston model with supporting application code and APIs.

In addition, the library supports low-level functions, such as random number generator (RNG), singular value decomposition (SVD), and tridiagonal and pentadiagonal matrix solvers.


Version 1.0
-----------
