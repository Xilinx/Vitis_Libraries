.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

******************************
GeoSpatial K-nearest Neighbors
******************************

Overview
========

K Nearest Neighbors (KNN) query is one of the most useful geospatial operations, which targets to solve problems like "What are the 10 nearest taxi trip pickup points of New York Time Square?". 
The underlying algorithm is straightforward, including compute the distances between query location and spatial objects, sorting objects by the distances, and return the top-k.  One typical input format of geospatial data is CSV, thus a csv parser to extract the spatial object is necessary in the design.

Kernel Implemention
===================

The overall diagram of a KNN kernel is shown in the following figure:

.. image:: /images/knn_diagram.png
    :align: center

Building Blocks:

- CSV Parser (DataAnalytic Library L1 Primitive) :ref:`guide-csv-parser`: Parse input CSV file according to the schema configurations, output spatial object coordinate (x, y), and index.
- Distance: Compute the distance between base location and spatial object location; Euclidean distance is applied.
- `Sort Top-K (Graph Library L1 Primitive) <https://docs.xilinx.com/r/en-US/Vitis_Libraries/graph/guide_L1/primitives/sortTopK.html>`_: Sort the distance in ascending order, and keep top-k objects.

End2End Performance
===================

Result Prerequisite: 

- Dataset: `TLC Trip Record Data <https://www1.nyc.gov/site/tlc/about/tlc-trip-record-data.page>`_ 2016-01 yellow taxi trip record. This dataset has 10,906,858 trip records, and column pickup_longtitude and pickup_latitude are used as spatial object inputs.
- AMD Device: U.2.
- Geopandas is run on a server with Intel® Xeon® CPU E5-2667 v3 @ 3.20 GHz and PyGeos package applied.
- E2E Execution time includes the read CSV file to memory, compute distance, and sort distance.
- Top K = 5.
- Number of CSV Parser PU = 2 or 4: Considering the fact that processing ability of a single CSV Parser PU is a maximum one byte per cycle, which is the bottleneck of design, two or four CSV Parser PUs are instantiated to enhance the performance.

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
