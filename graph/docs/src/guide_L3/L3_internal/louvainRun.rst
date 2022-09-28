.. 
   Copyright 2021 Xilinx, Inc.
  
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
  
       http://www.apache.org/licenses/LICENSE-2.0
  
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

********************************
Louvain Modularity Launch Demo
********************************

To load and launch louvain Modularity for multi compute units on multi boards, we provided louvainRun API with two kernel modes which command is -kernel_mode. 

* LOUVAINMOD_PRUNING_KERNEL, -kernel_mode 1, launch the pre-build L2 u50 1 cu kernel on multi boards.
* LOUVAINMOD_2CU_U55C_KERNEL, -kernel_mode 2, launch the pre-build L2 u55c 2 cu kernel on multi boards parallelly. 

Launch u50 Flow
#####################

Ensure run the script L3/tests/pre_launch.sh to set the path of libgraphL3.so
Ensure export the right $(PROJECTPATH) for the L2 pre-build U50 xclbin path, this xclbin may need 777 permission by xrm.

.. code-block:: sh

	cd L3/tests/louvainRun
	make host
	./build_dir.sw_emu.xilinx_u50_gen3x16_xdma_201920_3/host.exe -x $(PROJECTPATH)/build_dir.sw_emu.xilinx_u50_gen3x16_xdma_201920_3/kernel_louvain.xclbin -kernel_mode 1 -num_devices 1 -devices xilinx_u50_gen3x16_xdma_201920_3 -num_level 100 -num_iter 100 -load_alveo_partitions ../louvainPartition/example_tx.par.proj -setwkr 0 -driverAlone 
    
Louvain fast Input Arguments:

.. code-block:: sh

   Usage: host.exe -[-kernel_mode -x -num_devices -devices -num_level -num_iter -load_alveo_partitions -setwkr -driverAlone]
         -kernel_mode:                 the kernel mode  : 1 is u50 1 cu, 2 is for u55c 2cu parallel launch in louvainRunSubGraph
         -x:                           path of the xclbin : path
         -num_devices:                 the number of boards : 1~n
         -devices:                     the shell name of device : xilinx_u50_gen3x16_xdma_201920_3 xilinx_u55c_gen3x16_xdma_base_2
         -num_level:                   the max number of level or phase the louvain modularity : 1~n
         -num_iter:                    the max number of iteration the louvain modularity ：1~n
         -load_alveo_partitions：      the project to be load : *.par.proj
         -setwkr                       the number Client nodes in CS modes, currently is 0 : 0
         -driverAlone                  only use the Server node in CS modes.

Launch u55c Flow
#####################

Ensure run the script L3/tests/pre_launch.sh to set the path of libgraphL3.so
Ensure export the right $(PROJECTPATH) for the L2 pre-build U55C xclbin path, this xclbin may need 777 permission by xrm.

.. code-block:: sh

	cd L3/tests/louvainRun
	make host
	./build_dir.sw_emu.xilinx_u55c_gen3x16_xdma_base_2/host.exe -x $(PROJECTPATH)/build_dir.sw_emu.xilinx_u55c_gen3x16_xdma_base_2/kernel_louvain.xclbin -kernel_mode 2 -num_devices 1 -devices xilinx_u55c_gen3x16_xdma_base_2 -num_level 100 -num_iter 100 -load_alveo_partitions ../louvainPartition/example_tx.par.proj -setwkr 0 -driverAlone 





