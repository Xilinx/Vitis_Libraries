.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

****************************************************
Random Forest (training)
****************************************************

Overview
========

Random forest or random decision forest is an ensemble learning method for classification, regression, and other tasks that consisting of a multitude of decision trees at training time and outputting the class that is the mode of the classes (classification) or mean prediction (regression) of the individual trees. Random forest uses bagging and feature randomness to create some uncorrelated trees to prevent decision tree's overfitting to their training set.

Basic Algorithm
================

To ensure the diversity of each individual tree, Random Forest uses two methods:

Sample Bagging:
Random forest allows each individual tree to randomly select instances from the dataset with a replacement, resulting in different trees. This process is known as bagging.

Feauture Bagging:
In decision tree, consider every possible feature and pick the one that gets the most gains when splitting each tree node. In contrast, each tree in a random forest can pick only from a random subset of features. This forces even more variation amongst the trees in the model and ultimately results in lower correlation across trees and more diversification.

.. caution:: 
  The current implementation provides without-replacement sample bagging to complete rf framework quickly. When compared with spark, also choose the same bagging method. With-replacement bagging will come in later updates.

Implementation
=======================

The random forest framework is shown in the following figure:

.. _my-figure1:
.. figure:: /images/rf/rf_framework.png
    :alt: Figure 1 Random Framewok architecture on FPGA
    :width: 80%
    :align: center

Separate random forest int sampling module and decision tree module.
In sampling module, you design some different implementations of bagging methods, each of which implements different sampling methods.
Instance sampling: withoutplacement instance sampling, sampling ratio can be set by user.
                   In particular, withplacement instance sampling is implemented by multi withoutplacement instance samplings.
Feature Quantize: To save bandwith and memory, you learn from spark binned dataset, quantize each feature into a fixed point integer.

.. _my-figure2:
.. figure:: /images/rf/rf_quantize.png
    :alt: Figure 2 Quantize modules on FPGA
    :width: 80%
    :align: center
 
In quantize method, each feauture value is binned into a fixed point integer; for example, if a feature value is 0.45, while the feature splitting array is [-0.1,0.25,0.50,0.80], the binning rule includes:
   a. < -0.1        -> 0
   b. -0.1 ~ 0.25   -> 1
   c. 0.25 ~ 0.50   -> 2
   d. 0.50 ~ 0.80   -> 3
   e. > 0.80        -> 4

so after quantizing, the binned feature value is 2 (0,25<0.45<0.5).
In the quantize module, the main part is searching the inverval of each value, Figure 2 shows a detailed implementation by unrolling a binary search tree.

In the decision tree module, feature sampling support is added, so that each tree point reserves its own feature subsets. When splitting a tree node, it can only select the best feature from the specific subset. 
In current version, decision tree in random forest implements the quntize optimization for more kernel frequency and performance. You can get a special random forest with one single tree to implement a decision tree. Actually, decision tree from this method can ease IO bound compared with a non-quantized decision tree.


.. _my-figure3: 
.. figure:: /images/rf/rf_header1.png
    :alt: Figure 3 RF data header on FPGA
    :width: 80%
    :align: center

.. _my-figure4:
.. figure:: /images/rf/rf_header2.png
    :alt: Figure 4 DT data header on FPGA
    :width: 80%
    :align: center

Figure 3 and Figure 4 shows the data layout in the ddr buffer.
In Figure 3, you reserve last 288 bits in the `data` header for multi-seeds, by setting `seedid`, the function read corresponding seed from the header. After one call is done, the corresponding seed will write back an updated seed. The trick is for multi-sampling module kernel calls and PCIe® data write only once. 

.. _my-figure5:
.. figure:: /images/rf/rf_pipelined.png
    :alt: Figure 3 random forest tree based ping-pong mult-kernels calls
    :width: 80%
    :align: center
    
In general, you can only put 1~4 individual trees on board. Figure 5 shows the host implementaion mode of a random forest tree. In this mode, you can overlap the PCIe read, kernel call, and PCIe write, making the most of the kernel/hardware resources.

.. caution:: 
  The current rf decision tree has the same limitations with the orignal decision tree. For thousands of features, the current framework cannot support all features saving in an individual tree, so a feature sampling module is implemented for further extention.

Resource Utilization
====================

The hardware resources are listed in the following table. This is for the demonstration as configured two groups of rf_sampling + rf_decisiontree (in four super logic regions (SLRs)), achieving a 180 MHz clock rate.

You can configure the number of engines in a build. 

========================== ============ ============ ============ ============ ============= =============
  Name                      LUT          LUTAsMem     REG          BRAM         URAM          DSP        
========================== ============ ============ ============ ============ ============= =============
  User Budget                1727040      790560       3454080      2688         1280          12288       
                              [100.00%]    [100.00%]    [100.00%]    [100.00%]    [100.00%]     [100.00%] 
     Used Resources          622427       85078        830719       785          854           94       
                              [ 36.04%]    [ 10.76%]    [ 24.05%]    [ 29.20%]    [ 66.72%]     [  0.76%] 
     Unused Resources        1104613      705482       2623361      1903         426           12194       
                              [ 63.96%]    [ 89.24%]    [ 75.95%]    [ 70.80%]    [ 33.28%]     [ 99.24%] 
     RandomForestKernel_1    622427       85078        830719       785          854           94       
                              [ 36.04%]    [ 10.76%]    [ 24.05%]    [ 29.20%]    [ 66.72%]     [  0.76%] 
========================== ============ ============ ============ ============ ============= =============




.. toctree::
   :maxdepth: 1
