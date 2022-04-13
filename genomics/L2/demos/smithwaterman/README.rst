=========================================
Xilinx Smithwaterman Genomics
=========================================
The design contain hardware accelerated implementation of Smith-
Waterman algorithm. The Smith-Waterman algorithm performs local 
sequence alignment for determining similar regions between two 
strings of nucleic acid sequences or protein sequences. The 
Smith-Waterman algorithm compares segements of all possible 
lengths and optimizes the similarity measure.
 


Smithwaterman example resides in ``L2/demos/smithwaterman`` directory. 

Follow build instructions to generate host executable and binary.

The binary host file generated is named as "**xil_sw**" and it is present in ``./build`` directory.

Executable Usage
----------------

To execute single Smithwaterman Kernel  : ``./build/xil_sw -xbin ./build/xclbin_<xsa_name>_<TARGET mode>/krnl_smithwaterman.xclbin``
       
Results
-------

Resource Utilization 
~~~~~~~~~~~~~~~~~~~~~

Table below presents resource utilization of Xilinx Smithwaterman Genomics
kernels. The final Fmax achieved is 300MHz 

Genomics
~~~~~~~~~~~

=============    ====== ====== ===== ==== ==== 
Flow               LUT  LUTMem REG   BRAM URAM 
=============    ====== ====== ===== ==== ==== 
Smithwaterman     172K   11K   136K   2    0              
=============    ====== ====== ===== ==== ==== 


Performance Data
~~~~~~~~~~~~~~~~

Table below presents kernel throughput achieved for a single compute
unit. 

============================= =========================
Topic                         Results
============================= =========================
Genomics Throughput            267GCUPS                                                      
============================= =========================

Note: Overall throughput can still be increased with multiple compute.


Standard Smithwaterman Support
-------------------------------

This application is compatible with standard Smithwaterman application (Genomics).  
