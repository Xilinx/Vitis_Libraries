.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.
.. _guide-kMeansTain:

********************************
Internals of kMeansTaim
********************************

.. toctree::
      :hidden:
   :maxdepth: 2

This document describes the structure and execution of kMeansTrain, implemented as a :ref:`kMeansPredict <cid-xf::ml::kMeansTrain>` function.

.. image:: /images/kMeansTrain.png
      :alt: k-means Tainning Structure
   :width: 80%
   :align: center

kMeansTrain fits new centers based native k-means using the existing samples and initial centers you provide. To achieve to accelertion training, DV elements in a sample are input at the same time and used for computing distance with KU centers and updating centers. The static configures are set by template parameters and dynamic by arguments of the API in which dynamic ones should not greater than static ones. 

There are applicable conditions:

1. Dim*Kcluster should be less than a fixed value. For example, Dim*Kcluster<=1024*1024 for centers with float stored in URAM and 1024*512 for double on an AMD Alveo™ U250.

2. KU and DV should be configured properly due to limitation to URAM. For example, KU*DV=128 when centers are stored in URAM on an Alveo U250.

3. The dynamic confugures should close to static ones to void unuseful computing inside.

.. CAUTION:: These Applicable conditions.

Benchmark

The following results are based on:
     1) Dataset from UCI;
         a) https://archive.ics.uci.edu/dataset/371/nips+conference+papers+1987+2015
         b) http://archive.ics.uci.edu/ml/datasets/Human+Activity+Recognition+Using+Smartphones
         c) http://archive.ics.uci.edu/ml/datasets/US+Census+Data+%281990%29
     2) All data as double are processed;
     3) Unroll factors DV=8 and KU=16;
     4) Results compared to Spark 2.4.4 and initial centers from Spark to ensure the same input;
     5) Spark 2.4.4 is deployed in a server which has 56 processers(Intel® Xeon® CPU E5-2690 v4 @ 2.60 GHz)

   
Training Resources (Device: Alveo U250)
============================================

====== ======= ======== ========== ======== ======== ====== ======= 
  D       K      LUT     LUTAsMem    REG       BRAM    URAM    DSP  
====== ======= ======== ========== ======== ======== ====== ======= 
 5811    80     295110   50378      371542    339     248     420   
------ ------- -------- ---------- -------- -------- ------ ------- 
 561     144    260716   26016      371344    323     152     420   
------ ------- -------- ---------- -------- -------- ------ ------- 
 68     2000    255119   24295      372487    309     168     425   
====== ======= ======== ========== ======== ======== ====== ======= 

Training Performance (Device: Alveo U250)
============================================

====== ======= ======== ============= ============== ============== =============== =============== ============ ===========
  D       K    samples    1 thread      8 Threads      16 Threads     32 Threads      48 Threads      FPGA         FPGA  
                         on spark(s)   on Spark(s)     on Spark(s)    on Spark(s)     on Spark(s)    Execute(s)   Freq(MHz)
====== ======= ======== ============= ============== ============== =============== =============== ============ ===========
 5811    80      11463    93.489         49.857          49.860         48.001          50.875         29.410       202
                          (3.17X)        (1.69X)         (1.63X)        (1.89X)         (1.72X)         (1X)    
------ ------- -------- ------------- -------------- -------------- --------------- --------------- ------------ -----------
 561     144     7352      10.781         6.557           6.546          6.216          6.190          2.136        269
                          (5.04X)        (3.06X)         (3.06X)        (2.91X)         (2.89X)         (1X)
------ ------- -------- ------------- -------------- -------------- --------------- --------------- ------------ -----------
 68     2000    857765    547.001        173.116         170.217       161.169          166.214        158.903      239
                          (3.44X)        (1.08X)         (1.07X)       (1.01X)          (1.04X)         (1X)
====== ======= ======== ============= ============== ============== =============== =============== ============ ===========
