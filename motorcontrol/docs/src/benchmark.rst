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


.. Project documentation master file, created by
   sphinx-quickstart on Thu Jun 20 14:04:09 2022.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

==========
Benchmark 
==========
    

Performance Summary for APIs
------------------------------

Table 1 : Summary table for performance and resources of APIs

.. table:: Table 1 Summary table for performance and resources of APIs
    :align: center

    +------------------------------+----------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+
    |  API                         |   Flow   | Input bandwith         | Output bandwith        | Freq.  | LUT    | BRAM| URAM| DSP   |
    +==============================+==========+========================+=======+========+=======+========+========+=====+=====+=======+
    |  FOC                         | export IP|         20M/s          |         20M/s          | 100MHz |  5179  |  2  |  0  |  67   |
    +------------------------------+----------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+
    |  FOC_sensorless              | export IP|         13M/s          |         13M/s          | 100MHz |  6569  |  4  |  0  |  100  |
    +------------------------------+----------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+
    |  SVPWM_DUTY                  | export IP|         1M/s           |         1M/s           | 100MHz |  2101  |  0  |  0  |   7   |
    +------------------------------+----------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+
    |  PWM_GEN                     | export IP|         100M/s         |         100M/s         | 100MHz |  1094  |  0  |  0  |   6   |
    +------------------------------+----------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+
    |  QEI                         | export IP|         100M/s         |         2.5M/s         | 100MHz |  867   |  0  |  0  |   5   |
    +------------------------------+----------+------------------------+-------+--------+-------+--------+--------+-----+-----+-------+



These are details for benchmark result and usage steps.

.. toctree::
   :maxdepth: 1

   Sensor_based_FOC <benchmark/FOC_sensor.rst>
   Sensor_less_FOC <benchmark/FOC_sensorless.rst>
   QEI <benchmark/QEI.rst>
   SVPWM_DUTY <benchmark/SVPWM_DUTY.rst>
   PWM_GEN <benchmark/PWM_GEN.rst>

Test Overview
--------------

Here are benchmarks of the Vitis Motor Control Library using the Vitis environment and comparing with cpu(). 

.. _l2_vitis_motorcontrol:


* **Download code**

These Motor Control benchmarks can be downloaded from `vitis libraries <https://github.com/Xilinx/Vitis_Libraries.git>`_ ``main`` branch.

.. code-block:: bash

   git clone https://github.com/Xilinx/Vitis_Libraries.git 
   cd Vitis_Libraries
   git checkout main
   cd motorcontorl 

* **Setup environment**

Specifying the corresponding Vitis, XRT, and path to the platform repository by running following commands.

.. code-block:: bash

   source <intstall_path>/installs/lin64/Vitis/2022.2/settings64.sh
   source /opt/xilinx/xrt/setup.sh
   export PLATFORM_REPO_PATHS=/opt/xilinx/platforms
