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

from ctypes import *
import numpy as np
import sys
import argparse
import os

class XFBLASManager:
  def __init__(self,libFile):
    self._lib = cdll.LoadLibrary(libFile)
    self._lib.xfblasCreate.argtypes = [c_char_p,c_char_p,c_char_p,c_uint,c_uint]
    self._lib.xfblasCreate.restype = c_bool
    self._lib.xfblasSend.argtypes = [np.ctypeslib.ndpointer(flags="C_CONTIGUOUS"),c_ulonglong,c_uint,c_uint,c_uint]
    self._lib.xfblasSend.restype = c_bool
    self._lib.xfblasGet.argtypes = [np.ctypeslib.ndpointer(flags="C_CONTIGUOUS"),c_uint,c_uint]
    self._lib.xfblasGet.restype = c_bool
    self._lib.xfblasGemm.argtypes = [c_uint,c_uint,c_uint,c_uint,
                                     np.ctypeslib.ndpointer(flags="C_CONTIGUOUS"), c_uint,
                                     np.ctypeslib.ndpointer(flags="C_CONTIGUOUS"), c_uint,
                                     c_uint,
                                     np.ctypeslib.ndpointer(flags="C_CONTIGUOUS"), c_uint,
                                     c_uint,c_uint]
    self._lib.xfblasGemm.restype = c_bool
    self._lib.xfblasGemv.argtypes = [c_uint,c_uint,c_uint,
                                     np.ctypeslib.ndpointer(flags="C_CONTIGUOUS"), c_uint,
                                     np.ctypeslib.ndpointer(flags="C_CONTIGUOUS"), c_uint,
                                     c_uint,
                                     np.ctypeslib.ndpointer(flags="C_CONTIGUOUS"), c_uint,
                                     c_uint,c_uint]
    self._lib.xfblasGemv.restype = c_bool
    self._lib.xfblasFcn.argtypes = [c_uint,c_uint,c_uint,c_uint,
                                     np.ctypeslib.ndpointer(flags="C_CONTIGUOUS"), c_uint,
                                     np.ctypeslib.ndpointer(flags="C_CONTIGUOUS"), c_uint,
                                     c_uint,
                                     np.ctypeslib.ndpointer(flags="C_CONTIGUOUS"), c_uint,
                                     np.ctypeslib.ndpointer(flags="C_CONTIGUOUS"), c_uint,
                                     c_int,c_int,
                                     c_short,c_short,
                                     c_uint,c_uint]
    self._lib.xfblasGemm.restype = c_bool
    
  def createGemm(self,xclbin,numKernel,idxDevice):
    open('xrt_log.txt', 'a').close()
    b_xclbin = xclbin.encode('utf-8')
    b_log = xclbin.encode('utf-8')
    return self._lib.xfblasCreate(b_xclbin,b'Gemm',b'xrt_log.txt',numKernel,idxDevice)
    
  def createGemv(self,xclbin,numKernel,idxDevice):
    open('xrt_log.txt', 'a').close()
    b_xclbin = xclbin.encode('utf-8')
    b_log = xclbin.encode('utf-8')
    return self._lib.xfblasCreate(b_xclbin,b'Gemv',b'xrt_log.txt',numKernel,idxDevice)
  
  def createFcn(self,xclbin,numKernel,idxDevice):
    open('xrt_log.txt', 'a').close()
    b_xclbin = xclbin.encode('utf-8')
    b_log = xclbin.encode('utf-8')
    return self._lib.xfblasCreate(b_xclbin,b'Fcn',b'xrt_log.txt',numKernel,idxDevice)
    
  def sendMat(self,A,idxKernel,idxDevice):
    return self._lib.xfblasSend(A,c_ulonglong(A.size),c_uint(A.itemsize),idxKernel,idxDevice)
    
  def getMat(self,A,idxKernel,idxDevice):
    return self._lib.xfblasGet(A,idxKernel,idxDevice)
    
  def gemmOp(self,A,B,C,idxKernel,idxDevice):
    return self._lib.xfblasGemm(c_uint(A.shape[0]), c_uint(A.shape[1]), c_uint(B.shape[1]), 1, A, c_uint(A.shape[1]), B, c_uint(B.shape[1]), 1, C, c_uint(B.shape[1]),idxKernel,idxDevice)
  
  def gemvOp(self,A,x,y,idxKernel,idxDevice):
    return self._lib.xfblasGemv(c_uint(A.shape[0]), c_uint(A.shape[1]),1, A, c_uint(A.shape[1]), x, 1, y, 1, idxKernel,idxDevice)
  
  def fcnOp(self,A,B,C,X,postScale,postShift,preluScale,preluAlpha,idxKernel,idxDevice):
    return self._lib.xfblasFcn(c_uint(A.shape[0]), c_uint(A.shape[1]), c_uint(B.shape[1]), 1, A, c_uint(A.shape[1]), B, c_uint(B.shape[1]), 1, C, c_uint(C.shape[1]),X,c_uint(X.shape[1]),postScale,postShift,preluScale,preluAlpha,idxKernel,idxDevice)
  
  
_xfblasManager = None
  
def createGemm(args,xclbin_opts,numKernel=1,idxDevice=0):
    if int(xclbin_opts['GEMX_runGemm'])!= 1:
        raise Exception('The xclbin does not include gemm engine.')
    createManager(args.lib)
    return _xfblasManager.createGemm(args.xclbin,numKernel,idxDevice)
  
def createGemv(args,xclbin_opts,numKernel=1,idxDevice=0):
    if int(xclbin_opts['GEMX_runGemv'])!= 1:
        raise Exception('The xclbin does not include gemv engine.')
    createManager(args.lib)
    return _xfblasManager.createGemv(args.xclbin,numKernel,idxDevice)
  
def createFcn(args,xclbin_opts,numKernel=1,idxDevice=0):
    if int(xclbin_opts['GEMX_runFcn'])!= 1:
        raise Exception('The xclbin does not include fcn engine.')
    createManager(args.lib)
    return _xfblasManager.createFcn(args.xclbin,numKernel,idxDevice)
  
def sendMat(A,idxKernel=0,idxDevice=0):
    return _xfblasManager.sendMat(A,idxKernel,idxDevice)
  
def getMat(A,idxKernel=0,idxDevice=0):
    return _xfblasManager.getMat(A,idxKernel,idxDevice)
  
def gemmOp(A,B,C,idxKernel=0,idxDevice=0):
    return _xfblasManager.gemmOp(A,B,C,idxKernel,idxDevice)
  
def gemvOp(A,x,y,idxKernel=0,idxDevice=0):
    return _xfblasManager.gemvOp(A,x,y,idxKernel,idxDevice)
  
def fcnOp(A,B,C,X,postScale=1,postShift=0,preluScale=1,preluAlpha=0,idxKernel=0,idxDevice=0):
    return _xfblasManager.fcnOp(A,B,C,X,postScale,postShift,preluScale,preluAlpha,idxKernel,idxDevice)
  
def createManager ( libFile ):
  global _xfblasManager
  if not _xfblasManager:
    _xfblasManager = XFBLASManager(libFile)    
  return True  
  
def parse_cfg(filename):
    myvars = {}
    with open(filename) as myfile:
        for line in myfile:
            for word in line.split():
               name, var = word.split("=")
               myvars[name.strip()] = var.rstrip()  
    return myvars

def default_args():
    parser = argparse.ArgumentParser(description='xfblas')
    parser.add_argument('--xclbin', required = True, help='file path to FPGA bitstream')
    parser.add_argument('--lib', required = True, help='file path to xfblas shared library')
    parser.add_argument('--cfg', required=True, help='file describing .xclbin properties')
    return parser
      
def processCommandLine():
    parser = default_args()
    args = parser.parse_args()
    xclbin_opts = parse_cfg ( args.cfg ) 
    return args, xclbin_opts