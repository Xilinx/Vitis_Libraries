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

.. meta::
   :keywords: BLAS, Library, Vitis BLAS Library, Vitis BLAS, Keras MLP, Acceleration
   :description: Keras is Python based machine learning framework. It provides high level neural network APIs.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _keras_application_l3: 
 
Vitis BLAS L3 based Keras MLP Acceleration
===========================================

1. Introduction
------------------
Keras is Python based machine learning framework. It provides high level neural network APIs. It can run on top of other low level neural network frameworks for numerical computations. Vitis BLAS L3 Python APIs could be used for full connected Keras model and could provide better performance compared to CPU result. 

In L3/applications/MLP/keras/simple, an example of using Keras to do classification on a simple model showed that how to use Vitis BLAS L3 Python APIs in Keras applications.

2. Python Classes for using Keras
-----------------------------------

.. autoclass:: keras_rt.KerasRT
    :members:
    
.. autoclass:: xfblas_L3_rt.XfblasRT
    :members:



3. Steps to run the application
--------------------------------
- Build shared library
- set PYTHONPATH
- find the path to the xclbin and run the command (currently U200 and U250 are supported, xclbin could be downloaded in `Vitis BLAS library xclbins`_ )

.. _Vitis BLAS library xclbins:  https://www.xilinx.com/bin/public/openDownload?filename=vitis_BLAS_library_r1.0_xclbin.tar

.. code-block:: bash
  
  source /opt/xilinx/xrt/setup.sh
  cd L3/src/sw/python_api/
  make 
  cd ../../../applications/MLP/keras/simple/
  export PYTHONPATH=../../../../src/sw/python_api:./
  python mlp.py --data data/SansEC_Train_Data.csv --model best_model.h5 --xclbin PATH_TO_FCN_XCLBIN/gemx.xclbin --cfg PATH_TO_FCN_XCLBIN/config_info.dat --lib ../../../../src/sw/python_api/lib/xfblas.so
  
3. details
------------
This Keras application is using xclbin that included FCN (Fully Connected Network) engine. FCN engine is based on GEMM operation with post scales and pRelu supported.

- FCN :

1. C = pRelu(((A * B + X) * alpha) >> beta; where A, B, X and C are dense matrices, alpha and beta are integers.
2. pRelu: for each c in C; c = (c < 0)? ((c * pRelu_alpha) >> pRelu_beta): c; where pRelu_alpha and pRelu_beta are integers.

The following two packages are required for using Vitis BLAS L3 Python APIs:

.. code-block:: python

  import xfblas_L3 as xfblas
  import mlp_common

Only predict is done by FPGA, so CPU pre-trained 3-layer model is saved in best_model.h5, users could also set option --train to True to re-train the model.

Option --run_async is for the case when xclbin has multiple compute units, users could set it to True to run those CUs in parallel. For larger input sizes, this will bring better performance compared to run one by one. Currently, 2 CUs could be put in U200 xclbin, and 4 CUs could be put in U250 xclbins.

In the mlp.py, users first need to do the initialization, one fpga_rt is required for one CU, in the following code, model is the Keras Sequential model, its weight matrices are loaded from given model file best_model.h5. xclbin_opts is the paramater loaded from config_info.dat. 

.. code-block:: python

    fpga_rt = []  
    fpga_out = []
    for i in range(numKernels):
      fpga_rt.append(mlp_common.init_fpga(model,xclbin_opts, g_wgt_scale, g_bias_scale, g_post_scale,None,i,0))

After initialization, input sample need to be sent to FPGA for predict, and results will be returned to fpga_out. 

For non-async case, simply call predict function will do the job.

.. code-block:: python

    for i in range(numKernels):
        fpga_out.append(fpga_rt[i].predict(inp, g_in_scale))

For async case, 

.. code-block:: python

    for i in range(numKernels):
        fpga_rt[i].send_matrices(inp, None)
    
    xfblas.executeAsync(numKernels,0)

    for i in range(numKernels):
        fpga_out.append(fpga_rt[i].get_result())

In the given example, the activation function of the last layer is softmax, so CPU softmax function is called after getting the results from FPGA. Users could apply different activation function after getting results from FPGA.