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
import math
import xfblas_L3 as xfblas
import time

class XfblasRT():
  '''
  base class for using the XFBLAS in Keras
  
  Parameters
  
  xclbin_opts: dictionary 
             information read from config_info.dat used to build the xclbin  
  wgt:         list
             weights of the model
  bias:        list
             bias of the model
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
  
  def __init__(self, xclbin_opts, wgt, bias, wgt_scale, bias_scale, post_scale,relu_scale,idxKernel,idxDevice):
      ddrwidth = int(xclbin_opts["GEMX_ddrWidth"])
      self.min_m = ddrwidth * int(xclbin_opts["GEMX_gemmMBlocks"])
      self.min_k = ddrwidth * int(xclbin_opts["GEMX_gemmKBlocks"])
      self.min_n = ddrwidth * max (int(xclbin_opts["GEMX_gemmKBlocks"]), int(xclbin_opts["GEMX_gemmNBlocks"]) )
      if type (wgt) != list:
          wgt = [wgt]
      
      if type(bias) != list:
          bias = [bias]
     
      self._wshape = []
      self.offset_list = [2]
      for w in wgt:
          self._wshape.append(w.shape)
      if xclbin_opts["GEMX_dataType"] == "float":
          self._qw = wgt
          self.bias = bias
      else:
          self._qw = [np.int16(np.around(a*b)) for a,b in zip(wgt, wgt_scale)]
          self.bias = [np.int32(np.around(a*b)) for a,b in zip(bias, bias_scale)]
          
      for i,b in enumerate(self._qw):
          self._qw[i] = self.format_for_fpga( b, self.min_m, self.min_k)
          xfblas.sendMat(self._qw[i],idxKernel,idxDevice)
          self.offset_list.append(self.get_offset(self._qw[i]))
          
      self.fpga_buf = []
      self._qb = []
      self.out_dim = None
      self.post_scale = post_scale
      self.relu_scale = relu_scale
      self.xclbin_opts = xclbin_opts
      self.idxKernel = idxKernel
      self.idxDevice = idxDevice
      
  def get_offset(self,w):
      return int(w.shape[0]*w.shape[1]*w.itemsize/4096+self.offset_list[-1])
  
  def get_padded_shape ( self, shape, min_row, min_col):
      row_padded = int( math.ceil( np.float32(shape[0]) / min_row ) * min_row ) 
      col_padded = int( math.ceil( np.float32(shape[1]) / min_col ) * min_col )
      return row_padded,col_padded

  def format_for_fpga ( self, nparr, min_row, min_col):    
      row_padded, col_padded = self.get_padded_shape ( nparr.shape, min_row, min_col)
      padded_arr = np.zeros ( (row_padded, col_padded), dtype=nparr.dtype, order='C')
      padded_arr[0:nparr.shape[0], 0:nparr.shape[1]] = nparr
      return padded_arr            
    
  def format_bias (self, b, dim, min_row, min_col):
      if b.ndim == 1:
          b = np.broadcast_to(b, (dim[0],dim[1]) )
      
      b = self.format_for_fpga( b, min_row, min_col)
      xfblas.sendMat(b,self.idxKernel,self.idxDevice)
      self.offset_list.append(self.get_offset(b))
      return b
    
  def init_fpgabuf (self, in_shape ):  
      fpga_buf = []
      buf_dim = [in_shape]
  
      for i in self._wshape:
          buf_dim.append( (in_shape[0], i[1]) )
          
      self.out_dim = buf_dim[-1]
          
      for d in buf_dim:
          d_padded = self.get_padded_shape(d, self.min_m, self.min_n)
          inter_mat = np.zeros ( d_padded, dtype=self._qw[0].dtype, order='C')
          fpga_buf.append(inter_mat)
          
      self.fpga_buf = fpga_buf
      
      formatted_bias = []
      for dim,b  in zip (buf_dim[1:], self.bias):
          b = self.format_bias (b, dim, self.min_m, self.min_n)
          formatted_bias.append(b)   
      
      self._qb = formatted_bias
    
  def loadInstr(self):
      xfblas.freeInstr(self.idxKernel,self.idxDevice)
      for i,(w_i,b_i) in enumerate( zip( self._qw, self._qb) ):
          xfblas.fcnOp( w_i , self.fpga_buf[i], self.fpga_buf[i+1], b_i, self.post_scale[i][0], self.post_scale[i][1],1,0,self.idxKernel,self.idxDevice)
            
  def predict_old ( self, inp, in_scale): #keep this for debug
      self.init_fpgabuf(inp.shape)
      if self.xclbin_opts["GEMX_dataType"] == "float":
        padded_arr = self.format_for_fpga(inp, self.min_k, self.min_n)
        np.copyto(self.fpga_buf[0],  padded_arr, casting='same_kind', where=True)
      else:
        padded_arr = self.format_for_fpga(inp * in_scale, self.min_k, self.min_n)
        np.copyto(self.fpga_buf[0],  np.int16(np.around(padded_arr)), casting='same_kind', where=True)
      for i in self.fpga_buf:
        xfblas.sendMat(i,self.idxKernel,self.idxDevice)
      self.loadInstr()
      xfblas.getMat (self.fpga_buf[-1],self.idxKernel,self.idxDevice)
      for i in self.fpga_buf:
        xfblas.freeMat(i,self.idxKernel,self.idxDevice)
      for i in self._qb:
        xfblas.freeMat(i,self.idxKernel,self.idxDevice)
      return self.fpga_buf[-1][:self.out_dim[0],:self.out_dim[1]]
      
  def predict ( self, inp, in_scale):
      '''
      Return output prediction for the input sample
      
      Parameters
      
      inp
            input sample
      in_scale
            scale of input sample
      '''
      self.init_fpgabuf(inp.shape)
      if self.xclbin_opts["GEMX_dataType"] == "float":
        padded_arr = self.format_for_fpga(inp, self.min_k, self.min_n)
        np.copyto(self.fpga_buf[0],  padded_arr, casting='same_kind', where=True)
      else:
        padded_arr = self.format_for_fpga(inp * in_scale, self.min_k, self.min_n)
        np.copyto(self.fpga_buf[0],  np.int16(np.around(padded_arr)), casting='same_kind', where=True)
      for i in self.fpga_buf:
        self.offset_list.append(self.get_offset(i))
      xfblas.sendMat(self.fpga_buf[0],self.idxKernel,self.idxDevice)
      self.loadInstrByAddress()
      xfblas.execute(self.idxKernel,self.idxDevice)
      xfblas.getMatByAddress (self.fpga_buf[-1],self.offset_list[-2],self.idxKernel,self.idxDevice)
      xfblas.freeMat(self.fpga_buf[0],self.idxKernel,self.idxDevice)
      for i in self._qb:
        xfblas.freeMat(i,self.idxKernel,self.idxDevice)
      self.offset_list=self.offset_list[:len(self._qw)+1] # only keep the offset for weights
      return self.fpga_buf[-1][:self.out_dim[0],:self.out_dim[1]]

  def send_matrices(self, inp, in_scale):
      '''
      send input matrix and bias matrices to FPGA
      
      Parameters
      
      inp
            input sample
      in_scale
            scale of input sample
      '''
      self.init_fpgabuf(inp.shape)
      if self.xclbin_opts["GEMX_dataType"] == "float":
        padded_arr = self.format_for_fpga(inp, self.min_k, self.min_n)
        np.copyto(self.fpga_buf[0],  padded_arr, casting='same_kind', where=True)
      else:
        padded_arr = self.format_for_fpga(inp * in_scale, self.min_k, self.min_n)
        np.copyto(self.fpga_buf[0],  np.int16(np.around(padded_arr)), casting='same_kind', where=True)
      for i in self.fpga_buf:
        self.offset_list.append(self.get_offset(i))
      xfblas.sendMat(self.fpga_buf[0],self.idxKernel,self.idxDevice)
      self.loadInstrByAddress()
      
  def single_execute(self):
      xfblas.execute(self.idxKernel,self.idxDevice)
     
  def get_result(self):
      '''
      Return output prediction for the input sample
      '''
      xfblas.getMatByAddress (self.fpga_buf[-1],self.offset_list[-2],self.idxKernel,self.idxDevice)
      xfblas.freeMat(self.fpga_buf[0],self.idxKernel,self.idxDevice)
      for i in self._qb:
        xfblas.freeMat(i,self.idxKernel,self.idxDevice)
      self.offset_list=self.offset_list[:len(self._qw)+1] # only keep the offset for weights
      return self.fpga_buf[-1][:self.out_dim[0],:self.out_dim[1]]