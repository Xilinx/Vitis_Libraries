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

************************
GMAC Mode
************************

.. toctree::
   :maxdepth: 1

Overview
========

GMAC is a specialization of the GCM(Galois/Counter mode) and used for authentication only since its input data are not encrypted.


Implementation on FPGA
======================

The implementation of GMAC is based on GCM including generateEKY0 and tag_gen.
The input key and IV(initial vector) are converted to eky0 which is one input of tag_gen.
The module tag_gen generates the value of GMAC.

.. image:: /images/gmac_detail.png
   :alt: GMAC
   :width: 100%
   :align: center


Encryption Performance(Device: U250)
============================================

====== ======= ======= ===== ====== ===== ====== ========
 CLB     LUT     FF     DSP   BRAM   SRL   URAM   CP(ns)
====== ======= ======= ===== ====== ===== ====== ========
 4397   22422   16739    0     26    818    0     2.808
====== ======= ======= ===== ====== ===== ====== ========

