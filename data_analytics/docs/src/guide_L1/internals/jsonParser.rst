.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.


.. _guide-json-parser:

******************************
JSON Parser
******************************

JSON is a popular file format used to store and transmit data objects consisting of key-value pairs. The hardware JSON parser we provide can be used to accelerate the explanation process of the key-value pairs that are pre-stored in memory; in other words, it converts row data into the column format. Also, all the necessary information that used to indicate the column data of the parsed values will be packaged into the standard object-stream.

Features
=============================

Currently, the hardware JSON parser supports the following six data types: bool, integer (64-bit), string, float, double, and date. This parser does not support data type inference, so you should always provide the correct setting for each column in the input schema before initiating the acceleration process. It is also important to know that you need the nested structure of the specific key in the schema to parse the nested-object in the JSON lines. For example, if you have a JSON format like:

.. code-block:: json

    {
        "nested" :
        {
            "object" :
            {
                "bool column" : [true, null, false, false]
            },
            "int column" : 503
        },
        "float column" : 3.1415,
        "double column" : 0.99999,
        "date column" : "1970-01-01",
        "string column" : "hello world"
    }

Then, to let the hardware JSON parser work correctly, the schema should be specified as:

.. code-block:: cpp

    "nested/object/bool column": 0 // 0 denotes the bool type
    "nested/int column": 1 // 1 denotes the integer (64-bit) type
    "float column": 2 // 2 denotes the single-precision floating-point type
    "double column": 3 // 3 denotes the double-precision floating-point type
    "date column": 4 // 4 denotes the date (in string) type
    "string column": 5 // 5 denotes the string type

For the input JSON file, each JSON line should be stored compactly without any padding and seperated by **\"\\n\"** character. The hardware implementation will divide the whole input JSON file evenly and feed them into the individual processing unit (PU). Besides the flattened key-value pairs, the nested-object and the array on the leaf node is also welcomed by the hardware implementation. Meanwhile, the hardware parser will automatically detect the array and label each element with a specific index; no redundant information is needed in the input schema. Moreover, in one JSON line, the absence of some keys are allowed; the hardware parser will figure out each of them and then fill a null flag into the output object-stream.

Limitations
=============================

1. Array is only supported on leaf node, which means the nested array or the array of object is not supported.

2. Maximum supported number of fields is 16.

3. Maximum supported number of lines in each JSON file is 512k.

4. Maximum key length for each field is 256 bytes (included **\"/\"** as nested object separator).

5. Maximum supported length of string is 4k bytes.

Overall Structure
============================

Hardware JSON parser is implemented as multi-PU architecture to provide high throughput. The dataflow diagram of the kernel can be illustrated as follows:

.. image:: /images/json_parser.png
   :alt: JSON Parser Diagram
   :width: 80%
   :align: center

The whole JSON file should first be preloaded to a compacted buffer. For the parallel execution of each PU, the reading block will automatically divide the input file into several chunks by its size. Line parser is a FSM-based module to parse out each key-value pair at the throughput of 1 byte/cycle. For array on leaf node, it labels each element with an incremental index; the last element will be labeled with all F's to indicate the end of the array. For each value with a different data type, there is one dedicated parse-unit to translate the raw bytes into its own value. At the final stage, each selected field will be merged into one full column before structuring into the output object-stream protocol. Each row in the selected output columns corresponds to the specific input JSON line, so the missing key/element of a specific JSON line will be indicated by a `null` object.
