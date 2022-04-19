=============================
Xilinx SMEM Genomics
=============================

Smithwaterman demo resides in ``L2/tests/smem`` directory.

Xilinx SMEM Algorithm is FPGA based implementation of
standard SMEM. Xilinx implementation of SMEM application                          
is aimed for achieving high throughput. This Xilinx Smithwaterman 
application is developed and tested on Xilinx Alveo U250. 


Results
-------

Resource Utilization 
~~~~~~~~~~~~~~~~~~~~~

Table below presents resource utilization of Xilinx Smithwaterman Genomics
kernels. The final Fmax achieved is 300MHz 

======= ===== ====== ===== ===== ===== 
Flow    LUT   LUTMem REG   BRAM  URAM 
======= ===== ====== ===== ===== ===== 
SMEM    46K     5K   60K    70    70    
======= ===== ====== ===== ===== =====  

Performance Data
~~~~~~~~~~~~~~~~

Table below presents kernel throughput achieved for a single compute
unit. 

========================= =========================
Topic                       Results
========================= =========================
Average DRAM Bandwidth       2 GB/s 
========================= =========================

Note: Overall throughput can still be increased with multiple compute
units.

Software & Hardware
-------------------

::

     Software: Xilinx Vitis 2021.2
     Hardware: xilinx_u200_xdma_201830_2 (Xilinx Alveo U250)

Executable Usage
----------------
 
1. To execute single file for genomics  : ``./build/xil_smem ./build/xclbin_<xsa_name>_<TARGET mode>/<smem.xclbin>  <file_name>``
           
    
      - Note: Default arguments are set in Makefile
                                          
