 # Copyright 2019 Xilinx, Inc.
 #
 # Licensed under the Apache License, Version 2.0 (the "License");
 # you may not use this file except in compliance with the License.
 # You may obtain a copy of the License at
 #
 #     http://www.apache.org/licenses/LICENSE-2.0
 #
 # Unless required by applicable law or agreed to in writing, software
 # distributed under the License is distributed on an "AS IS" BASIS,
 # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 # See the License for the specific language governing permissions and
 # limitations under the License.

import numpy as np

from keras_rt import KerasRT

def predict_cpu (model, test_data):
    predictions = model.predict(test_data)
    return predictions
  
def init_fpga(model, xclbin_opts, g_wgt_scale, g_bias_scale, g_post_scale,relu_scale=None,idxKernel=0,idxDevice=0):
    fpga_rt = KerasRT(model, xclbin_opts, g_wgt_scale, g_bias_scale, g_post_scale,relu_scale,idxKernel,idxDevice)
    return fpga_rt

def predict_fpga( fpga_rt, test_data, g_in_scale):
    result = fpga_rt.predict(test_data, g_in_scale)
    result = result.astype(np.float32)
    #run softmax on CPU
    for i in range(result.shape[0]):
        result[i,:] = softmax(result[i,:])   
    return result
    
def softmax(x):
    """Compute softmax values for each sets of scores in x."""
    e_x = np.exp(x - np.max(x))
    return e_x / e_x.sum()

def compare_real_results ( expected, actual):
    e_r = np.around(expected,decimals=3)
    a_r = np.around(actual, decimals=3)
    if np.array_equal (e_r, a_r):
        print ("SUCCESS!!!")
    else:
        diff = e_r - a_r
        num_diff = 0
        for i in range(diff.shape[0]):
            #print (e_r[i],a_r[i])
            if e_r[i]!=a_r[i]:
                num_diff=num_diff+1
        print('num_diff:',num_diff,'num_same:',diff.shape[0]-num_diff)
        np.savetxt("out.np", a_r, fmt="%d")
        np.savetxt("out_golden.np", e_r, fmt="%d")
        np.savetxt("diff.np", e_r - a_r, fmt="%d")

def compute_standlone(inp,wb):
    weights = wb[0::2]
    bias = wb[1::2]
    number_of_layers = len(weights)
    C=[inp]
    for i in range(number_of_layers):
      o = np.matmul (C[i], weights[i])
      o = o + bias[i]
      if i != len(weights) -1 :
          o[o<0] = 0
      C.append(o)    
    return C[-1]