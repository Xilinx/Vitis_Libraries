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

from __future__ import print_function
import numpy as np
import pandas as pd
from keras.utils import np_utils
from sklearn.preprocessing import LabelEncoder
from keras.models import Sequential
from keras.layers import Dense
from keras.callbacks import ModelCheckpoint
import argparse
import sys
import xfblas_L3 as xfblas
import mlp_common

#Quantization parameters to bring fp32 ranges to fit into int16; parameters are derived offline ( see quantize.py )
#This parameters won't be used if using fp32 xclbin
g_in_scale = 31.130533916017484
g_wgt_scale = [159879.54672570297, 135942.35558298964, 72420.85217502648]
g_bias_scale = [159879.54672570297, 135942.35558298964, 72420.85217502648]
g_post_scale = [[1, 17], [1, 17], [5, 24]]

def train(train_fd, predictors, train_data, num_classes):
    
    # We will use a multi-layer perceptron classification model for random search.
    # Create model
    model = create_keras_model(len(predictors), num_classes )
    # Compile model
    model.compile(loss='categorical_crossentropy', optimizer='adam', metrics=['accuracy'])
    modelcheckpoint_callback = ModelCheckpoint("./best_model.h5", monitor='val_loss',mode='min', save_best_only=True, save_weights_only=True)
    model.fit(train_fd[predictors], train_data, epochs=200, batch_size=50, callbacks=[modelcheckpoint_callback], validation_split=0.20, shuffle=True)

def predict_hwemu ( weights, test_data, num_classes):
    model = create_keras_model(test_data.values.shape[1], num_classes )
    model.load_weights(weights)
    return compute_standalone_hwemu( test_data.values, model.get_weights())

def compute_dense(weight, bias, inp, scalein=1, post_scale=1):
    scaledin = inp*scalein
    inp16 = np.int16(scaledin)#input from previous stage to 16bits
    m64 = np.matmul(np.int64(inp16), np.int64(weight))#intermediate accumulation to 64 bits
    
    output64 = m64
    if bias is not None:
        bias64 = np.int64(bias)#bias to 64 bits
        output64 = m64 + bias64
        
    o64d = output64/(2**post_scale[1])
    #o64d = output64/post_scale[1]
    o64m = o64d*post_scale[0]
    output = np.int16(o64m)#scale down for 16 bits
    return output
    
def compute_standalone_hwemu( inp, wb ):
    weights = wb[0::2]
    bias = wb[1::2]

    #quantization
    w_int16 = [ np.int16(a*b) for a,b in zip(weights, g_wgt_scale)]
    b_int32 = [ np.int32(a*b) for a,b in zip(bias, g_wgt_scale)]
        
    o1 = compute_dense ( w_int16[0], b_int32[0], inp, g_in_scale, g_post_scale[0])
    #print ("o1 range (", np.min(o1), ",", np.max(o1), ")")
    o1[o1 < 0] = 0
    
    o2 = compute_dense ( w_int16[1], b_int32[1], o1, 1, g_post_scale[1])
    #print ("o2 range (", np.min(o2), ",", np.max(o2), ")")    
    o2[o2 < 0] = 0
    o3 = compute_dense ( w_int16[2], b_int32[2], o2, 1, g_post_scale[2])
    #print ("o3 range (", np.min(o3), ",", np.max(o3), ")")
    #softmax
    for i in range(o3.shape[0]):
        o3[i,:] = mlp_common.softmax(np.float64(o3[i,:]))
    return o3

def compute_standalone( inp, wb ):
    print ("inp (", np.min(inp), ",", np.max(inp))
    for i,w in enumerate(wb):
        print ( "w", i, ": ", np.min(w), ", ", np.max(w))
        
    o1 = np.matmul ( inp, wb[0])
    o1 = o1 + wb[1]
    print ("o1 (", np.min(o1), ",", np.max(o1))
    o1[o1 < 0] = 0

    o2 = np.matmul ( o1, wb[2])
    o2 = o2 + wb[3]
    print ("o2 (", np.min(o2), ",", np.max(o2))    
    o2[o2 < 0] = 0
    o3 = np.matmul ( o2, wb[4])
    print ("o3 (", np.min(o3), ",", np.max(o3))    
    o3 = o3 + wb[5]
    #softmax
    for i in range(o3.shape[0]):
        o3[i,:] = mlp_common.softmax(np.float64(o3[i,:]))
    return o3
 
def compare_results ( expected, actual):
    e_r = np.around(expected,decimals=3)
    a_r = np.around(actual, decimals=3)
    if np.array_equal (e_r, a_r):
        print ("SUCCESS!!!")
    else:
        diff = e_r - a_r
        num_diff = 0
        for i in range (e_r.shape[0]):
            if not np.array_equal( e_r[i,:] , a_r[i,:]):
                print("line", i+1, "is different")
                num_diff += 1
                
        print ( num_diff , "/", e_r.shape[0], "incorrect")
        np.savetxt("out.np", a_r, fmt="%f")
        np.savetxt("out_golden.np", e_r, fmt="%f")
        np.savetxt("diff.np", diff, fmt="%f")
    
def create_keras_model(in_dims, num_classes):
    '''
    Generate a simple Keras model.
    '''  
    model = Sequential()
    model.add(Dense(100, input_dim=in_dims, activation='relu', name='d1'))
    model.add(Dense(25, activation='relu', name='d2'))
    model.add(Dense(num_classes, activation='softmax', name='d3'))
    model.summary()
    return model
if  __name__ == '__main__':
    np.random.seed(27)
    
    parser = xfblas.default_args()
    parser.add_argument('--data', required = True, help='inference data file')
    parser.add_argument('--model', required = True, help='model')
    parser.add_argument('--train', default = False, help='set to True if retrain the model')
    parser.add_argument('--run_async', default = False, help='run async for multi-kernel')
    
    
    args = parser.parse_args()
    xclbin_opts = xfblas.parse_cfg(args.cfg)
    
    numKernels = int(xclbin_opts["GEMX_numKernels"])

    #load xclbin 
    xfblas.createFcn(args,xclbin_opts,numKernels,0)
   
    train_fd = pd.read_csv(args.data) # Load training data.
    IDcol = 'Run' # A column used to identified the run for data collection; not an independent variable.
    target = 'Class' # The column name for our dependent variable.
    predictors = [x for x in train_fd.columns if x not in [target, IDcol]] # Define column names to use as independent variables.
    
    # Encode class values as integers
    encoder = LabelEncoder()
    encoder.fit(train_fd[target])
    encoded_Y = encoder.transform(train_fd[target])
    # Convert integers to dummy variables (i.e. one hot encoded)
    train_y = np_utils.to_categorical(encoded_Y)

    num_classes =  len(train_fd[target].unique())    
    model = create_keras_model(train_fd[predictors].values.shape[1],num_classes)
    model.load_weights(args.model)
    
    if args.train:
        train(train_fd, predictors, train_y, len(train_fd[target].unique()))
      
    fpga_rt = []  
    fpga_out = []
    for i in range(numKernels):
      fpga_rt.append(mlp_common.init_fpga(model,xclbin_opts, g_wgt_scale, g_bias_scale, g_post_scale,None,i,0))
        
    inp = train_fd[predictors].values
    
    if not args.run_async: # for larger batch size, run multi-kernels in parallel will bring up to 4x better performance
        for i in range(numKernels):
            fpga_out.append(mlp_common.predict_fpga(fpga_rt[i], inp, g_in_scale))
    else:
        for i in range(numKernels):
            fpga_rt[i].send_matrices(inp, None)
    
        xfblas.executeAsync(numKernels,0)

        for i in range(numKernels):
            fpga_out.append(fpga_rt[i].get_result())
   
            fpga_out[i] = fpga_out[i].astype(np.float32) 
            for j in range(fpga_out[i].shape[0]):
                fpga_out[i][j,:] = mlp_common.softmax(fpga_out[i][j,:])
    
    cpu_out = mlp_common.predict_cpu( model, train_fd[predictors].values)
    
    for i in range(numKernels):
        compare_results ( cpu_out, fpga_out[i])

    xfblas.destroy(numKernels,0)
    