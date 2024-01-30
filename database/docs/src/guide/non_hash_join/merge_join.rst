.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Merge-Join, Merge-Left-Join, mergeJoin, mergeLeftJoin, combineCol, mergeJoin, mergeLeftJoin
   :description: Describes the structure and execution of Merge-Join and Merge-Left-Join.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _guide-merge_join:

************************************************
Internals of Merge-Join and Merge-Left-Join 
************************************************

.. toctree::
   :hidden:
   :maxdepth: 1

This document provides a user guide and describes the structure of Merge-Join and Merge-Left-Join, implemented as a :ref:`mergeJoin<cid-xf::database::mergeJoin>` function and :ref:`mergeLeftJoin<cid-xf::database::mergeLeftJoin>` function, respectively.

----------
User Guide
----------

When calling the ``mergeJoin``/ ``mergeLeftJoin`` function, you need to set the key type and payload type. Only one key stream and one payload stream is given for an input table. If multiple key columns or multiple payload columns are required, use the :ref:`combineCol <cid-xf::database::combineCol>` to combine columns.

You need to push the input tables into the related streams. You also need to configure the function to merge the ascend/descend tables by setting the ``isascend`` parameter to true/false.

.. CAUTION:: The left table should not contain duplicated keys.
..

The left and right result tables are pushed out in separate stream. If required, use the :ref:`combineCol <cid-xf::database::combineCol>` to combine the left and right table into one stream.

.. Important:: The mergeLeftJoin function has a isnull_strm output stream to mark if the result right table is null (the current left key does not exist in the right table).

---------
Structure
---------

Use the merge join the ascend tables as an example:

.. image:: /images/merge_join.png
   :alt: Merge Join Structure
   :align: center
..

Every clock cycle, compare the keys from the left and right tables:

- If the keys are not the same, pull the stream with a smaller key and no output.
- If the keys are the same, pull the right stream and push the keys and payloads to the output stream.