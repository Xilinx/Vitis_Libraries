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

******************************
JSON Parser
******************************

JSON is a popular file format used to store and transmit data objects consisting of key-value pairs. JSON parser can be used to accelerate explaining the flatten key-value pairs in memory, just like the CSV parser. All the parsed value will be packaged into the standard object-stream including the missing key.

Features
=============================
Currently, JSON parser supports the following 6 data type: bool, integer, string, float, double and date. This parser don't support data type inference, so user should give the right setting for each column in the input schema defination like the CSV parser does.

For the input JSON file, each json line should be stored in one buffer compactly line by line without any padding. Currently, nested object is NOT supported. In one json line, the absent of some keys are allowed. And the parser would figure out each of them and then fill a none-type into the output object-stream.

Overall Structure
============================
JSON parser is implemented as multiple PU architecture to provide high throughput. The diagram is illustrated as below.

.. image:: /images/json_parser.png
   :alt: JSON Parser Diagram
   :width: 80%
   :align: center

The full JSON file should be loaded in a compacted buffer firstly. For the parallel execution of each PU, the read block will divide the input file into several chunks by its size. Line parser is a FSM-based module to parse-and-split out each key-value pair at one byte per cycle. Also, all the trivial characters will be removed in this stage. For each data type input, there is one dedicated parse-unit to translate the raw bytes into its own value. At the final stage, each selected field will be merged into one full column before structuring into the output object-stream protocal.

