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

class XfblasRT():
  
  def __init__(self, xclbin_opts, wgt, bias, wgt_scale, bias_scale, post_scale,relu_scale,idxKernel,idxDevice):
      ddrwidth = int(xclbin_opts["GEMX_ddrWidth"])
      self.min_m = ddrwidth * max (int(xclbin_opts["GEMX_gemmKBlocks"]), int(xclbin_opts["GEMX_gemmMBlocks"]) )
      self.min_k = ddrwidth * int(xclbin_opts["GEMX_gemmKBlocks"])
      self.min_n = ddrwidth * int(xclbin_opts["GEMX_gemmNBlocks"])
      if type (wgt) != list:
          wgt = [wgt]
      
      if type(bias) != list:
          bias = [bias]
     
      self._wshape = []
      for w in wgt:
          self._wshape.append(w.shape)
          
      if xclbin_opts["GEMX_dataType"] == "float":
          self._qw = wgt
          self._qb = bias
      else:
          self._qw = [np.int16(np.around(a*b)) for a,b in zip(wgt, wgt_scale)]
          self._qb = [np.int32(np.around(a*b)) for a,b in zip(bias, bias_scale)]
          
      for i,b in enumerate(self._qw):
          b = np.transpose(b)
          self._qw[i] = self.format_for_fpga( b, self.min_m, self.min_k)
          xfblas.sendMat(self._qw[i],idxKernel,idxDevice)
          
      self.fpga_buf = []
      self.out_dim = None
      self.post_scale = post_scale
      self.relu_scale = relu_scale
      self.xclbin_opts = xclbin_opts
      self.idxKernel = idxKernel
      self.idxDevice = idxDevice
  
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
          b = np.broadcast_to(b, (dim[1],dim[0]) )
      
      b = np.transpose(b)
      b = self.format_for_fpga( b, min_row, min_col)
      xfblas.sendMat(b,self.idxKernel,self.idxDevice)  
      return b
    
  def init_fpgabuf (self, in_shape ):  
      fpga_buf = []
      buf_dim = [in_shape]
  
      for i in self._wshape:
          buf_dim.append( (i[1], in_shape[1]) )
          
      self.out_dim = buf_dim[-1]
          
      for d in buf_dim:
          d_padded = self.get_padded_shape(d, self.min_m, self.min_n)
          inter_mat = np.zeros ( d_padded, dtype=self._qw[0].dtype, order='C')
          fpga_buf.append(inter_mat)
      self.fpga_buf = fpga_buf
      
      formatted_bias = []
      for dim,b  in zip (buf_dim[1:], self._qb):
          b = self.format_bias (b, dim, self.min_m, self.min_n)
          formatted_bias.append(b)   
      
      self._qb = formatted_bias           
    
  def loadInstr(self):
      xfblas.freeInstr(self.idxKernel,self.idxDevice)
      for i,(w_i,b_i) in enumerate( zip( self._qw, self._qb) ):
          xfblas.fcnOp( w_i , self.fpga_buf[i], self.fpga_buf[i+1], b_i, self.post_scale[i][0], self.post_scale[i][1],1,0,self.idxKernel,self.idxDevice)
            
  def predict ( self, inp, in_scale):
      inp = np.transpose(inp)
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
      return np.transpose(self.fpga_buf[-1][:self.out_dim[0],:self.out_dim[1]])   
      
      