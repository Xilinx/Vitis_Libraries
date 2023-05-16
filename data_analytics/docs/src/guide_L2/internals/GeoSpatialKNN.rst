.. 
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
  
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
GeoSpatial K-nearest Neighbors
******************************

Overview
========

K Nearest Neighbors(KNN) query is one of the most useful geospatial operations, 
which targets to solve problems like "What are the 10 nearest taxi trip pickup points of New York Time Square?". 
The underlying algorithm is straightforward, 
including compute the distances between query location and spatial objects, sorting objects by the distances and return the top-k. 
One typical input format of geospatial data is CSV, thus a csv parser to extract spatial object is necessary in the design.

Kernel Implemention
===================

The overall diagram of KNN kernel is shown in the figure below:

.. image:: /images/knn_diagram.png
    :align: center

Building Blocks:

- CSV Parser(DataAnalytic Library L1 Primitive) :ref:`guide-csv-parser`: parse input csv file according to schema configurations, output spatial object coordinate (x, y) and index.
- Distance: compute distance between base location and spatial object location; Euclidean distance is applied.
- `Sort Top-K(Graph Library L1 Primitive) <https://docs.xilinx.com/r/en-US/Vitis_Libraries/graph/guide_L1/primitives/sortTopK.html>`_ : sort distance in ascending order and keep top-k objects.

End2End Performance
===================
Result Prerequisite: 

- Dataset: `TLC Trip Record Data <https://www1.nyc.gov/site/tlc/about/tlc-trip-record-data.page>`_ 2016-01 yellow taxi trip record. This dataset has 10,906,858 trip records, and column pickup_longtitude and pickup_latitude are used as spatial object inputs.
- Xilinx Device: U.2.
- Geopandas is run on server with Intel(R) Xeon(R) CPU E5-2667 v3 @ 3.20GHz and PyGeos package applied.
- E2E Execution time includes read csv file to memory, compute distance, and sort distance.
- Top K = 5.
- Number of CSV Parser PU = 2 or 4: considering the fact that processing ability of single CSV Parser PU is maximum one byte per cycle, which is the bottleneck of design, 2 or 4 CSV Parser PUs are instantiated to enhance the performance.

+---------------------+-----------------------+
|                     | E2E Execution Time(s) |
+=====================+=======================+
| KNN(2 CSV Parser)   | 4.301                 |
+---------------------+-----------------------+
| KNN(4 CSV Parser)   | 2.448                 | 
+---------------------+-----------------------+
| Geopandas(1 thread) | 36.097                |
+---------------------+-----------------------+


Resource Utilization
====================

+-------------------+-------+----------+-------+-------+------+-------+
| Device U.2        | LUT   | LUTAsMem | REG   | BRAM  | URAM | DSP   |
+===================+=======+==========+=======+=======+======+=======+
| KNN(2 CSV Parser) | 25458 | 2350     | 27262 | 25    | 0    | 110   |
+-------------------+-------+----------+-------+-------+------+-------+
|                   | 6.58% | 1.55%    | 3.21% | 3.56% | 0    | 5.62% |
+-------------------+-------+----------+-------+-------+------+-------+
| KNN(4 CSV Parser) | 43458 | 3606     | 44255 | 43    | 0    | 208   |
+-------------------+-------+----------+-------+-------+------+-------+
|                   | 11.2% | 2.37%    | 5.21% | 6.12% | 0    | 10.6% |
+-------------------+-------+----------+-------+-------+------+-------+
