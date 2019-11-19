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

.. _test_l3:

=====================
L3 API test
=====================
XFBLAS level 3 provides an automatic test flow that could be run by simple one line command:

.. code-block:: bash

  python run_test.py --shell SHELL_NAME --operator OPERATOR_NAME1 ...
  
The whole test flow includes 3 parts: 1) build the executable 2) generate input files and golden reference using python numpy library and save in bin files 3) run hw and compare with the golden reference.
  
For each operator that is available in level 3, profile json file could be found in test/xf_blas/OPERATOR_NAME/profile.json. Users could modify values in that file to run tests with different data types, different matrix sizes, different value range and so on.

Following is an example json file for operator gemm:

.. code-block:: json

  {
    "dataTypes": [
      "int16",
      "float32"
    ],
    "op": "gemm",
    "matrixDims": [
      [128, 128, 128],
      [1024, 256, 256]
    ],
    "valueRange": [
      -1024,
      1024
    ]
  }
