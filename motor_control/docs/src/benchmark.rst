.. 
   Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
   SPDX-License-Identifier: X11
   
   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:
   
   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.
   
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   IN THE SOFTWARE.
   
   Except as contained in this notice, the name of Advanced Micro Devices 
   shall not be used in advertising or otherwise to promote the sale,
   use or other dealings in this Software without prior written authorization 
   from Advanced Micro Devices, Inc.


.. Project documentation master file, created by
   sphinx-quickstart on Thu Jun 20 14:04:09 2022.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

==========
Benchmark 
==========
    

Performance Summary for APIs
------------------------------


.. table:: Table 1 Summary table for performance and resources of APIs
    :align: center

    +------------------------------+----------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+
    |  API                         |   Flow   | Input bandwith         | Output bandwith        | Freq.  | LUT    | BRAM| URAM| DSP   |
    +==============================+==========+========================+=======+========+=======+========+========+=====+=====+=======+
    |  SVPWM_DUTY                  | export IP|         1M/s           |         1M/s           | 100MHz |  2101  |  0  |  0  |   7   |
    +------------------------------+----------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+
    |  PWM_GEN                     | export IP|         100M/s         |         100M/s         | 100MHz |  1094  |  0  |  0  |   6   |
    +------------------------------+----------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+
    



These are details for benchmark result and usage steps.

.. toctree::
   :maxdepth: 1

   SVPWM_DUTY <benchmark/SVPWM_DUTY.rst>
   PWM_GEN <benchmark/PWM_GEN.rst>

Test Overview
--------------

Here are the benchmarks of the AMD Vitis |trade| Motor Control Library using the Vitis environment. 

.. _l1_vitis_motorcontrol:


* **Download code**

These Motor Control benchmarks can be downloaded from `vitis libraries <https://github.com/Xilinx/Vitis_Libraries.git>`_ ``main`` branch.

.. code-block:: bash

   git clone https://github.com/Xilinx/Vitis_Libraries.git 
   cd Vitis_Libraries
   git checkout main
   cd motor_control 

* **Setup environment**

Specifying the corresponding Vitis, XRT, and path to the platform repository by running following commands.

.. code-block:: bash

   source <intstall_path>/installs/lin64/Vitis/2023.2/settings64.sh
   source /opt/xilinx/xrt/setup.sh
   export PLATFORM_REPO_PATHS=/opt/xilinx/platforms

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
