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

*******************
String Compare APIs
*******************

Overview
========
String Compare APIs includes a group of functions that compare the value of string to the given character pattern. 
The functions are originated from the usage of Database SQL string operators: =/!=, IN, LIKE, and NOT.

- `string_equal()` performs comparison and returns true if two strings are exactly same. 
- `string_in()` determines if the input string matches any value in a list.
- `string_like()` determines whether the input string matches a specific pattern that includes wildcard character.
  
  - "%abc%": input string contains 'abc' at any position.
  - "abc%": input string starts with 'abc'.
  - "%abc": input string ends with 'abc'.

Meanwhile, the negation of above three functions are also provided.

Implementation
==============
string EQUAL
------------

.. image:: /images/stringEqual.png
   :alt: String EQUAL Structure
   :width: 80%
   :align: center

string IN
---------

.. image:: /images/stringIN.png
   :alt: String IN Structure
   :width: 80%
   :align: center

string LIKE
-----------

.. image:: /images/stringLIKE.png
   :alt: String LIKE Structure
   :width: 80%
   :align: center

Performance and Resource
========================

- Performance and Resource are profiled with U50 card.
- LUT/FF/Clock Period are get from Vivado implementation report.
- Simulation time is get from Cosim with 1000 input test strings.    
  

string IN
---------

+----------------+------------+-------+------+----------------+------------------------+
|MAX_BASE_STR_LEN|NUM_BASE_STR|LUT    |FF    |Clock Period(ns)|RTL simultation time(ns)|
+================+============+=======+======+================+========================+
|       64       |    1       |  566  | 446  |       2.7      |          31939         |
+----------------+------------+-------+------+----------------+------------------------+
|       64       |    4       |  1357 | 1031 |       2.88     |          31014         |
+----------------+------------+-------+------+----------------+------------------------+
|       64       |    8       |  2418 | 1737 |       2.9      |          31257         |
+----------------+------------+-------+------+----------------+------------------------+
|       32       |    8       |  2408 | 1735 |       2.89     |          30920         |
+----------------+------------+-------+------+----------------+------------------------+

Note:

- Performance and resource for string EQUAL could refer to row NUM_BASE_STR=1. 


string LIKE
-----------

+----------------+--------+-------+------+----------------+------------------------+
|MAX_BASE_STR_LEN|BATCH_SZ|LUT    |FF    |Clock Period(ns)|RTL simultation time(ns)|
+================+========+=======+======+================+========================+
|       32       |    2   |  1850 | 2154 |       3.02     |          108478        |
+----------------+--------+-------+------+----------------+------------------------+
|       64       |    2   |  3717 | 4227 |       3.21     |          104617        |
+----------------+--------+-------+------+----------------+------------------------+
|       64       |    4   |  4512 | 3320 |       3.06     |          65282         |
+----------------+--------+-------+------+----------------+------------------------+
|       64       |    8   |  8167 | 3500 |       3.20     |          45609         |
+----------------+--------+-------+------+----------------+------------------------+



