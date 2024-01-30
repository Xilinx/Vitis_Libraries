.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

*************************
Decision Tree (Predict)
*************************

Overview
========

Decision Tree (Classification/Regression) is a model to predict the classification or regression value of given samples. In structures of Decision Tree, leaf nodes give class labels and internal nodes represent certain attributes. Decision tree is a supervised learning algorithm, divided into two stages of training and predict. In the training stage, each partition is chosen greedily by selecting the best split from a set of possible splits to maximize the information gained at a tree node. In the predict stage, a decision tree can clearly express the decision process, that is, after series of attributes testing, a sample finally reaches a leaf node.

Algorithm (predict)
--------------------

The predict stage is the process of making a decision using a decision tree. Starting from the root node, test the corresponding feature attributes in the item, and select the output branch according to its value until it reaches the leaf node. Finally, use the category value or regression value stored by the leaf node as the decision result.

The decision tree can be linearized into decision rules, where the outcome is the contents of the leaf node, and the conditions along the path form a conjunction in the if clause. In general, the rules have the form:
if condition1 and condition2 and condition3 then outcome.

Suppose :match:`N` is the number of samples, :match:`Nodes`, an array of special data structure, represents a decision tree.:math:`MAX_DEPTH` is max depth of a decision tree. 

The detailed algorithm:
Input: A sequence of samples and Decision Tree (marked as :math:`nodes`).
Output: A sequence of predict results.

For m from 1 to :math:`N`
  For k from 1 to :math:`MAX_DEPTH`
    if :math:`nodes[k]` is not leaf node
      test corresponding feature attributes and select a branch.
    else
      output the category or regression value of current node.
      break

Implementation (predict)
===========================

In decision tree predict, each sample read :math:`i` layer to select a branch to :math:`i+1` layer. The max loop number of each sample is :math:`MAX_DEPTH`. Based on the algorithm, the description of decision tree predict if provided by the following figure.

.. image:: /images/tree/dt_predict.png
   :alt: framework of decision tree predict
   :width: 80%
   :align: center

To eliminate the loop-carried dependency for memory port deficiency, you partition different layers of decision tree in differen URAM. In batch mode predict, each sample reads various URAM; each round has no dependence so that the initiation interval (II) could achieve to 1. It is present as follows:

.. image:: /images/tree/dt_predict_pip.png
   :alt: decision tree predict pipeline
   :width: 80%
   :align: center

Profiling
---------

For decision tree classification predict:

The hardware resources for Decision Tree with a max tree depth of 16:

    +--------------------------+----------+----------+----------+----------+-----------------+
    |          Engines         |    SRL   |   URAM   |    FF    |    LUT   | Clock Period(ns)|
    +--------------------------+----------+----------+----------+----------+-----------------+
    |  Decision Tree Predict   |   61620  |    17    |   68681  |   81673  |       2.900     |
    +--------------------------+----------+----------+----------+----------+-----------------+

The hardware resources for Decision Tree with a max tree depth of 20:

    +--------------------------+----------+----------+----------+----------+-----------------+
    |          Engines         |    SRL   |   URAM   |    FF    |    LUT   | Clock Period(ns)|
    +--------------------------+----------+----------+----------+----------+-----------------+
    |  Decision Tree Predict   |   78052  |    21    |   85734  |   102923 |       2.937     |
    +--------------------------+----------+----------+----------+----------+-----------------+

For decision tree regression predict:

The hardware resources for Decision Tree with a max tree depth of 10:

    +--------------------------+----------+----------+----------+----------+-----------------+
    |          Engines         |    SRL   |   URAM   |    FF    |    LUT   | Clock Period(ns)|
    +--------------------------+----------+----------+----------+----------+-----------------+
    |  Decision Tree Predict   |   18504  |    11    |   42994  |   31031  |       3.844     |
    +--------------------------+----------+----------+----------+----------+-----------------+

The correctness of Decision Tree Predict is verified by comparing results with Decision Tree predict results of Spark mllib.
