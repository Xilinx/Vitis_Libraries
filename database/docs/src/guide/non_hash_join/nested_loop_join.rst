.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Nested-Loop-Join, nestedLoopJoin
   :description: Describes the structure and execution of Nested-Loop-Join.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _guide-nested_loop_join:

**********************************
Internals of Nested-Loop-Join
**********************************

.. toctree::
   :hidden:
   :maxdepth: 1

This document provides a user guide and describes the structure of the Nested-Loop-Join, implemented as a :ref:`nestedLoopJoin<cid-xf::database::nestedLoopJoin>` function.

User Guide
----------

When calling the ``nestedLoopJoin`` function, you need to set the key type and payload type. Only one key stream and one payload stream is given for an input table. If multiple key columns or multiple payload columns are required, use the :ref:`combineCol <cid-xf::database::combineCol>` to combine columns.

Every left row will become an independent channel to compare with the right table. You need to set the number of channels by setting the ``CMP_NUM`` template parameter. 50 is a typical number for the ``CMP_NUM``. 

.. CAUTION:: A very large CMP_NUM (more than 120) can result in numerous resource.
..

You need to push the left and right tables into the associated streams. The number of rows of the left table should not exceed the predefined ``CMP_NUM``, but it can be less than the ``CMP_NUM``. Unused channels will generate an empty table (assert the end of table flag for one cycle) to the next module.

Structure
---------

.. image:: /images/nested_loop_join.png
   :alt: Nested Loop Join Structure
   :align: center
..

The following steps will be performed when the nested loop join function is called:
- Load the left table using shift registers. 
- Pull out right table row by row and compared with the left table.