.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _guide-csv-parser:

******************************
CSV Parser
******************************

Comma-separated values file is a popular format which simply delimited values by commas. The CSV parser aims to accelerate explaining each input row with a user-defined schema, and then a standard object-stream is used to egress all the row values of the specified columns.

Features
=============================
Currently, the CSV parser supports the following six data types: bool, integer, string, float, double, and date. This parser does not support data type inference, so you should give the right setting for each column in the input schema definition.

For the input CSV file, adjacent fields must be separated by a single comma (other delimiters are not supported), and any field can be quoted. 

.. note:: The nested comma must be quoted, and the embedded double-quote characters and line breaks is not being supported so far. Also, the leading and trailing spaces between fields, as well as the heading record is forbidden.

For output data, the CSV parser will allow you to select some columns, and only the selected columns would be outputted by the object-stream interface. 

Overall Structure
============================

The CSV parser is implemented as a multiple PU architecture to provide high throughput. The diagram is illustrated as follows.

.. image:: /images/csv_parser.png
   :alt: CSV Parser Diagram
   :width: 80%
   :align: center

The full CSV file should first be loaded in a compacted buffer. For the parallel execution of each PU, the read block will divide the input file into several chunks by its size. Line parser is a FSM-based module to parse out each field at one byte per cycle. Also, all the trivial characters will be removed in this stage. For each data type input, there is one dedicated parse-unit to translate the raw bytes into its own value. At the final stage, each selected field will be merged into one full column before structuring into the output object-stream protocol.

