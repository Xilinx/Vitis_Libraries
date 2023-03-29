.. 
   Copyright 2019-2020 Xilinx, Inc.
  
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
  
       http://www.apache.org/licenses/LICENSE-2.0
  
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

.. toctree::
   :hidden:
   :maxdepth: 1 

.. _brief:

==================================
Vitis Ultrasound Library Tutorial
==================================

Get and setup the Vitis Ultrasound Library
==========================================

Setup Environment
------------------------------------

.. code-block:: bash

   #!/bin/bash

   # setup vitis env
   source <Vitis_install_path>/Vitis/2023.1/settings64.sh 
   source <XRT_install_path>/2023.1/xbb/xrt/packages/setenv.sh
   export PLATFORM_REPO_PATHS=<path_to_platforms>

   # set petalinux related env
   export SYSROOT=<path_to_platforms>/sw/versal/xilinx-versal-common-v2023.1/sysroots/aarch64-xilinx-linux/
   export ROOTFS=<path_to_platforms>/sw/versal/xilinx-versal-common-v2023.1/rootfs.ext4
   export K_IMAGE=<path_to_platforms>/sw/versal/xilinx-versal-common-v2023.1/Image

Download the Vitis Ultrasound Library
--------------------------------------

.. code-block:: bash

   #!/bin/bash
   git clone https://github.com/Xilinx/Vitis_Libraries.git
   cd Vitis_Libraries/ultrasound

Run a L1 Example
=================

.. code-block:: bash

   #!/bin/bash
   cd L2/tests/absV        # absV is an example case. Please change directory to any other cases in L1/test if interested.
   make run TARGET=x86sim  # run the x86sim
   make run TARGET=aiesim  # run the aiesim

Run a L2 Example
=================

.. code-block:: bash

   #!/bin/bash
   cd L2/tests/apodization # apodization is an example case. Please change directory to any other cases in L2/test if interested.
   make run TARGET=x86sim  # run the x86sim
   make run TARGET=aiesim  # run the aiesim
   make run TARGET=sw_emu  # run sw_emu. Build host and run software emulation
   make run TARGET=hw_emu  # run hw_emu. Build host and run hardware emulation, this step would lauch petalinux
   make all TARGET=hw      # build hw

L2 APIs Input Arguments:

.. code-block:: bash

   Usage: host.exe -[xclbin  data]
          -xclbin: the kernel name
          -data: the path to the input data
          

Run a L3 Example
=================

.. code-block:: bash

   #!/bin/bash
   cd L3/demos/plane_wave  # plane_wave is an example case. Please change directory to any other cases in L3/test if interested.
   make run TARGET=x86sim  # run the x86sim
   make run TARGET=aiesim  # run the aiesim
   make run TARGET=sw_emu  # run sw_emu
   make run TARGET=hw_emu  # run hw_emu
   make all TARGET=hw      # build hw

L3 APIs Input Arguments:

.. code-block:: bash

   Usage: host.exe -[xclbin  data]
          -xclbin: the kernel name
          -data: the path to the input data

Example logs

.. code-block:: shell 

   INFO: Initializing ADF API...
   INFO: XAIEFAL: INFO: Resource group Avail is created.
   INFO: XAIEFAL: INFO: Resource group Static is created.
   INFO: XAIEFAL: INFO: Resource group Generic is created.
   INFO: file:data/start_positions.txt size:64
   INFO: file:data/directions.txt size:64
   INFO: file:data/samples_arange.txt size:128
   INFO: file:data/image_points.txt size:128
   INFO: file:data/image_points.txt size:128
   INFO: file:data/tx_def_delay_distance.txt size:128
   INFO: file:data/tx_def_delay_distance.txt size:128
   INFO: file:data/tx_def_ref_point.txt size:128
   INFO: file:data/tx_def_focal_point.txt size:128
   INFO: file:data/t_start.txt size:128
   INFO: file:data/apo_ref_0.txt size:64
   INFO: file:data/xdc_def_0.txt size:64
   INFO: file:data/apo_ref_1.txt size:64
   INFO: file:data/xdc_def_1.txt size:64
   INFO: file:data/image_points.txt size:128
   INFO: file:data/delay_from_PL.txt size:206
   INFO: file:data/xdc_def_positions.txt size:130
   INFO: file:data/sampling_frequency.txt size:136
   INFO: file:data/image_points.txt size:128
   INFO: file:data/apodization_reference.txt size:156
   INFO: file:data/apo_distance_k.txt size:128
   INFO: file:data/F_number.txt size:188
   INFO: file:data/P1.txt size:32
   INFO: file:data/P2.txt size:32
   INFO: file:data/P3.txt size:32
   INFO: file:data/P4.txt size:32
   INFO: file:data/P5.txt size:32
   INFO: file:data/P6.txt size:32
   INFO: Input memory0 virtual addr 0x0xffffac8bc000
   INFO: Input memory1 virtual addr 0x0xffffac8bb000
   INFO: Input memory2 virtual addr 0x0xffffac8ba000
   INFO: Input memory3 virtual addr 0x0xffffac8b9000
   INFO: Input memory4 virtual addr 0x0xffff8ee67000
   INFO: Input memory5 virtual addr 0x0xffff8ee66000
   INFO: Input memory6 virtual addr 0x0xffff8ee65000
   INFO: Input memory7 virtual addr 0x0xffff8ee64000
   INFO: Input memory8 virtual addr 0x0xffff8ee63000
   INFO: Input memory9 virtual addr 0x0xffff8ee62000
   INFO: Input memory10 virtual addr 0x0xffff8ee61000
   INFO: Input memory11 virtual addr 0x0xffff8ee60000
   INFO: Input memory12 virtual addr 0x0xffff8ee5f000
   INFO: Input memory13 virtual addr 0x0xffff8ee5e000
   INFO: Input memory14 virtual addr 0x0xffff8ee5d000
   INFO: Input memory15 virtual addr 0x0xffff8ee5c000
   INFO: Input memory16 virtual addr 0x0xffff8ee5b000
   INFO: Input memory17 virtual addr 0x0xffff8ee5a000
   INFO: Input memory18 virtual addr 0x0xffff8ee59000
   INFO: Input memory19 virtual addr 0x0xffff8ee58000
   INFO: Input memory20 virtual addr 0x0xffff8ee57000
   INFO: Input memory21 virtual addr 0x0xffff8ee56000
   INFO: Input memory22 virtual addr 0x0xffff8ee55000
   INFO: Input memory23 virtual addr 0x0xffff8ee54000
   INFO: Input memory24 virtual addr 0x0xffff8ee53000
   INFO: Input memory25 virtual addr 0x0xffff8ee52000
   INFO: Input memory26 virtual addr 0x0xffff8ee51000
   INFO: Input memory27 virtual addr 0x0xffff8ee50000
   INFO: Output memory0 virtual addr 0x 0xffff8ee4f000
   INFO: Output memory1 virtual addr 0x 0xffff8ee4e000
   INFO: Output memory2 virtual addr 0x 0xffff8ee4d000
   INFO: Output memory3 virtual addr 0x 0xffff8ee4c000
   INFO: Output memory4 virtual addr 0x 0xffff8ee4b000
   INFO: Output memory5 virtual addr 0x 0xffff8ee4a000
   INFO: input kernel complete
   INFO: output kernel complete
   INFO: graph init
   INFO: graph run
   INFO: graph end
   INFO: mm2s1 completed with status(4)
   INFO: mm2s2 completed with status(4)
   INFO: mm2s3 completed with status(4)
   INFO: mm2s4 completed with status(4)
   INFO: mm2s5 completed with status(4)
   INFO: mm2s6 completed with status(4)
   INFO: mm2s7 completed with status(4)
   INFO: mm2s8 completed with status(4)
   INFO: mm2s9 completed with status(4)
   INFO: mm2s10 completed with status(4)
   INFO: mm2s11 completed with status(4)
   INFO: mm2s12 completed with status(4)
   INFO: mm2s13 completed with status(4)
   INFO: mm2s14 completed with status(4)
   INFO: mm2s15 completed with status(4)
   INFO: mm2s16 completed with status(4)
   INFO: mm2s17 completed with status(4)
   INFO: mm2s18 completed with status(4)
   INFO: mm2s19 completed with status(4)
   INFO: mm2s20 completed with status(4)
   INFO: mm2s21 completed with status(4)
   INFO: mm2s22 completed with status(4)
   INFO: mm2s23 completed with status(4)
   INFO: mm2s24 completed with status(4)
   INFO: mm2s25 completed with status(4)
   INFO: mm2s26 completed with status(4)
   INFO: mm2s27 completed with status(4)
   INFO: mm2s28 completed with status(4)
   INFO: mm2s wait complete
   INFO: s2mm1completed with status(4)
   INFO: s2mm2completed with status(4)
   INFO: s2mm3completed with status(4)
   INFO: s2mm4completed with status(4)
   INFO: s2mm5completed with status(4)
   INFO: s2mm6completed with status(4)
   INFO: s2mm wait compete
   INFO: file:data/golden/image_points.txt size:128
   INFO: file:data/golden/delay_to_PL.txt size:32
   INFO: file:data/golden/focusing_output.txt size:32
   INFO: file:data/golden/samples_to_PL.txt size:32
   INFO: file:data/golden/apodization.txt size:32
   INFO: file:data/golden/C.txt size:128
   INFO: data/golden/image_points.txt
   INFO: data/golden/delay_to_PL.txt
   INFO: data/golden/focusing_output.txt
   INFO: data/golden/samples_to_PL.txt
   INFO: data/golden/apodization.txt
   INFO: data/golden/C.txt
   INFO: Releasing remaining XRT objects...
   INFO: Test Done, err_cnt:0
   INFO: INFO: TEST PASSED, RC=0
   INFO: INFO: Embedded host run completed.
   INFO: 
   INFO: Total test time: 12.821966409683228
   INFO: Power OFF complete.
   INFO: Test passed!
   INFO: Total time on board: 716.9993252754211