.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Direct-Aggregate, directGroupAggregate
   :description: Describes the structure and execution of the Direct-Aggregate module.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _guide-direct_aggregate:

********************************************************
Internals of Direct-Group-Aggregate
********************************************************

.. toctree::
   :hidden:
   :maxdepth: 2

This document describes the structure and execution of Direct-Aggregate, implemented as a :ref:`directGroupAggregate <cid-xf::database::directGroupAggregate>` function.

.. image:: /images/direct_aggr.png
   :alt: Direct Aggregate Structure
   :align: center

The Direct-Aggregate primitive is a single processing unit which can calculate the group aggregate efficiently. The current design uses one input channel through which a pair of key and payload can be passed in each cycle. When there is more than one input channel, you could duplicate the PU calculate the final result on the host side to reach the goals.

.. ATTENTION::
    Applicable conditions:

    1. The data width of combined key and payload is configurable, while distinct and on-chip storage scale linear relationship. The recommended use case is that the width of the key is not large, and key values are closely continuous. DIRECTW is the low significant bit of the key and is used as the addressing space for the aggregate. For example, when the width of Combined key is 200 bits, but only the low 12 bits are significant, and the depth of on-chip storage resource will be 4K. This design introduces some flexibility over addressing directly with a 200-bit key value.
    
    2. There are eight functions for calculating the payload. They are MAX, MIN, SUM, COUNTONE, AVG, VARIANCE, NORML1, and NORML2. Each one is represented by an input parameter:
   
    - xf::database::AOP_MAX
    - xf::database::AOP_MIN
    - xf::database::AOP_SUM
    - xf::database::AOP_COUNTONE
    - xf::database::AOP_MEAN
    - xf::database::AOP_VARIANCE 
    - xf::database::AOP_NORML1
    - xf::database::AOP_NORML2

    3. The primitive provides two APIs: one API provides a template defined aggregate function which means the calculation function cannot change in runtime; another API provides a runtime programmable solution and it can easily change the aggregate functions by changing its OP when calling the API.

.. CAUTION::
   The width of significant bits in group key is limited and predefined. It will consume more FPGA internal memory, such as block RAM and URAM, if a large significant bit is set.

This ``directAggregate`` primitive has only one port for key input and one port for payload input. If your tables are joined by multiple key columns or has multiple columns as payload, use :ref:`combineUnit <cid-xf::database::combineCol>` to merge the column streams, and use :ref:`splitUnit <cid-xf::database::splitCol>` to split the output to columns.