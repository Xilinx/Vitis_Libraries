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

.. _overview:

.. toctree::
   :hidden:

Overview
------------

Vitis Data Mover Library is an open-sourced Vitis library of exchanging data between PL and AIE.

This library provides **Programmable 4D Data-Mover** and **Static Data-Mover** to help generate kernel design and improve development efficiency.

**Programmable 4D Data-Mover** have 2 types of kernel: **4DCuboidRead** and **4DCuboidWrite** which provide both flexible access pattern and keep high performance between DDR and AIE.

**Static Data-Mover** has 9 types of kernel which help AIE connect with DDR / URAM and BRAM. Their access pattern is simple continously read / write.


License
-------

Licensed using the `Apache 2.0 license <https://www.apache.org/licenses/LICENSE-2.0>`_.

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

Trademark Notice
----------------

    Xilinx, the Xilinx logo, Artix, ISE, Kintex, Spartan, Virtex, Zynq, and
    other designated brands included herein are trademarks of Xilinx in the
    United States and other countries.
    
    All other trademarks are the property of their respective owners.


