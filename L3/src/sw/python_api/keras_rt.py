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
import xfblas_L3 as xfblas
from xfblas_L3_rt import XfblasRT

class KerasRT(XfblasRT):
    '''
    base class for using the XFBLAS in Keras
    
    Parameters
    
    keras_model: model 
               pre-trained model
    xclbin_opts: dictionary 
               information read from config_info.dat used to build the xclbin  
    wgt_scale:   list
               Quantization parameters multiple with weight matrices
    bias_scale:  list
               Quantization parameters multiple with bias matrices
    post_scale:  list
               Quantization parameters multiple with output matrices on FPGA side
    relu_scale: list
               Relu parameters multiple with BIAS matrices on FPGA side
    idxKernel
               index of CU
    idxDevice 
               index of device
    '''
    def __init__(self, keras_model, xclbin_opts, wgt_scale, bias_scale, post_scale, relu_scale,idxKernel,idxDevice):
      keras_w = keras_model.get_weights()[0::2]
      keras_b = keras_model.get_weights()[1::2]
      XfblasRT.__init__(self, xclbin_opts, keras_w, keras_b, wgt_scale, bias_scale, post_scale,relu_scale,idxKernel,idxDevice)
      self.kmodel = keras_model
       
    def loadInstr(self):
      xfblas.freeInstr(self.idxKernel,self.idxDevice)
      for i,l in enumerate(self.kmodel.layers):
          act = l.get_config()['activation']
          if self._qw[0].dtype == np.float32:
            if act == 'relu':
              xfblas.fcnOp( self.fpga_buf[i], self._qw[i], self.fpga_buf[i+1], self._qb[i], 1, 0, 0, 0, self.idxKernel,self.idxDevice)
            else:
              xfblas.fcnOp( self.fpga_buf[i], self._qw[i], self.fpga_buf[i+1], self._qb[i], 1, 0, 1, 0, self.idxKernel,self.idxDevice)      
          else:
            if act == 'relu':
              xfblas.fcnOp( self.fpga_buf[i], self._qw[i], self.fpga_buf[i+1], self._qb[i], self.post_scale[i][0], self.post_scale[i][1], 0, 0, self.idxKernel,self.idxDevice)
            else:
              xfblas.fcnOp( self.fpga_buf[i], self._qw[i], self.fpga_buf[i+1], self._qb[i], self.post_scale[i][0], self.post_scale[i][1], 1, 0, self.idxKernel,self.idxDevice)
              
    def loadInstrByAddress(self):
      '''
      send instructions to FPGA
      '''
      xfblas.freeInstr(self.idxKernel,self.idxDevice)
      numLayers= len(self.kmodel.layers)
      for i,l in enumerate(self.kmodel.layers):
          act = l.get_config()['activation']
          if self._qw[0].dtype == np.float32:
            if act == 'relu':
              xfblas.fcnOpByAddress(self.offset_list[2*numLayers+i],self.offset_list[i],self.offset_list[2*numLayers+i+1],self.offset_list[numLayers+i], self.fpga_buf[i], self._qw[i], self.fpga_buf[i+1], self._qb[i], 1, 0, 0, 0, self.idxKernel,self.idxDevice)
            else:
              xfblas.fcnOpByAddress(self.offset_list[2*numLayers+i],self.offset_list[i],self.offset_list[2*numLayers+i+1],self.offset_list[numLayers+i],self.fpga_buf[i], self._qw[i], self.fpga_buf[i+1], self._qb[i], 1, 0, 1, 0, self.idxKernel,self.idxDevice)      
          else:
            if act == 'relu':
              xfblas.fcnOpByAddress(self.offset_list[2*numLayers+i],self.offset_list[i],self.offset_list[2*numLayers+i+1],self.offset_list[numLayers+i],self.fpga_buf[i], self._qw[i], self.fpga_buf[i+1], self._qb[i], self.post_scale[i][0], self.post_scale[i][1], 0, 0, self.idxKernel,self.idxDevice)
            else:
              xfblas.fcnOpByAddress(self.offset_list[2*numLayers+i],self.offset_list[i],self.offset_list[2*numLayers+i+1],self.offset_list[numLayers+i],self.fpga_buf[i], self._qw[i], self.fpga_buf[i+1], self._qb[i], self.post_scale[i][0], self.post_scale[i][1], 1, 0, self.idxKernel,self.idxDevice)