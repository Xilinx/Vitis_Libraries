.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: combine-split-unit, combineCol, splitCol
   :description: Describes the structure and execution of the Combine-Split-Unit.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _guide-combine_split_unit:

********************************************************
Internals of Combine-Split-Unit
********************************************************

.. toctree::
   :hidden:
   :maxdepth: 1 

This document describes the structure and execution of a Combine-Split-Unit, implemented as a :ref:`combineCol <cid-xf::database::combineCol>` function and :ref:`splitCol <cid-xf::database::splitCol>` function.

.. image:: /images/combine_unit.png
   :alt: Combine Unit Structure
   :align: center

.. image:: /images/split_unit.png
   :alt: Split Unit Structure
   :align: center

The Combine Unit primitive is used to combine two or more streams into one wider stream, and the Split Unit is used to split one big stream into several thinner streams. Because of the different numbers of input streams of combineUnit/output streams of spiltUnit, four versions of the combine/split unit are provided, including:

- 2-stream-input combine unit

- 3-stream-input combine unit

- 4-stream-input combine unit

- 5-stream-input combine unit

- 2-stream-output split unit

- 3-stream-output split unit

- 4-stream-output split unit

- 5-stream-output split unit

For the combine unit, the input streams are combined from left to right, with the corresponding inputs from stream1 to streamN. (also known as, output stream = [input stream1, input stream2, ..., input streamN]).

For the split unit, the output streams are split from right to left, with the corresponding inputs from stream1 to streamN. (also known as, [output streamN, ..., output stream2, output stream1] = input stream). 

.. CAUTION::
    - All input/output streams are an ap_uint<> data type.
    - The maximum number of supported streams are five for both the combine and split unit. When the input/output stream numbers are more than five, the combination of two or more combine/split unit are required.