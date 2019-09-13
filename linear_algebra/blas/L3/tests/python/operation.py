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
import os

class OP_ERROR(Exception):
  def __init__(self, message):
    self.message = message

def dataGen(dataType, size, maxValue, minValue):
  mat = np.random.random(size) * (maxValue - minValue) + minValue
  mat = mat.astype(dataType)
  return mat

def write2Bin(data_array, file_name):
  with open(file_name, "wb") as f:
    data_array.tofile(f)
  
def write2Txt(data_array, file_name):
  np.savetxt(file_name,data_array)

class BLAS_L2():
  def writeBins(self,m,n,cnt):
    write2Bin(self.a_in, self.out_dir+"matA_in"+str(cnt)+"_"+str(m)+"_"+str(n)+".bin");    
    write2Bin(self.x_in, self.out_dir+"vecX_in"+str(cnt)+"_"+str(n)+"_1.bin");
    write2Bin(self.y_in, self.out_dir+"vecY_in"+str(cnt)+"_"+str(m)+"_1.bin");
    write2Bin(self.y_out, self.out_dir+"vecY_out"+str(cnt)+"_"+str(m)+"_1.bin");
    write2Bin(self.param, self.out_dir+"param_in"+str(cnt)+".bin");
    

class gemv(BLAS_L2):
  def __init__(self):
    print("***** Generating golden reference for GEMV ******")

  def genBin(self, cnt, dataType, cppDataType, size, maxValue, minValue):
    if not len(size) == 2:
        raise OP_ERROR("[ERROR] GEMV wrong matrix size: "+str(size))

    [m, n] = size;
    self.a_in = dataGen(dataType, [m, n], maxValue, minValue);
    self.x_in = dataGen(dataType, [n, 1], maxValue, minValue);
    # TODO, y_in is currently not supported with non-zero.
    self.y_in = dataGen(dataType, [m, 1], 0, 0);
    
    self.alpha = 1
    self.beta = 1
    
    self.y_out = self.compute();
    
    # transa, m, n, alpha, lda, incx, beta, incy, kernelIndex
    self.param = np.asarray([0, m, n, self.alpha, n, 1, self.beta, 1, 0], dtype=np.int32)    
    
    self.out_dir = "out_test/gemv/data/"+cppDataType+"/"
    if not os.path.exists(self.out_dir):
      os.makedirs(self.out_dir)
    self.writeBins(m,n,cnt)
    
  def compute(self):
    return self.alpha * np.matmul(self.a_in, self.x_in) + self.beta * self.y_in; 

class BLAS_L3():
  def writeBins(self,m,n,k,cnt):
    write2Bin(self.a_in, self.out_dir+"matA_in"+str(cnt)+"_"+str(m)+"_"+str(k)+".bin")  
    write2Bin(self.b_in, self.out_dir+"matB_in"+str(cnt)+"_"+str(k)+"_"+str(n)+".bin")
    write2Bin(self.c_in, self.out_dir+"matC_in"+str(cnt)+"_"+str(m)+"_"+str(n)+".bin")
    write2Bin(self.c_out, self.out_dir+"matC_out"+str(cnt)+"_"+str(m)+"_"+str(n)+".bin")
    write2Bin(self.param, self.out_dir+"param_in"+str(cnt)+".bin")
    
class gemm(BLAS_L3):
  def __init__(self):
    print("***** Generating golden reference for GEMM ******")
    
  def genBin(self, cnt, dataType, cppDataType, size, maxValue, minValue):
    if not len(size) == 3:
        raise OP_ERROR("[ERROR] GEMM wrong matrix size: "+str(size))

    [m, n, k] = size;
    self.a_in = dataGen(dataType, [m, k], maxValue, minValue);
    self.b_in = dataGen(dataType, [k, n], maxValue, minValue);
    self.c_in = dataGen(dataType, [m, n], maxValue, minValue);
    
    self.alpha = 1
    self.beta = 1
    
    self.c_out = self.compute();
    
    # transa, transb, m, n, k, alpha, lda, ldb, beta, ldc, kernelIndex
    self.param = np.asarray([0, 0, m, n, k, self.alpha, k, n, self.beta, n, 0], dtype=np.int32)
    
    self.out_dir = "out_test/gemm/data/"+cppDataType+"/"
    if not os.path.exists(self.out_dir):
      os.makedirs(self.out_dir)
    self.writeBins(m,n,k,cnt)

  def compute(self):
    return self.alpha * np.matmul(self.a_in, self.b_in) + self.beta * self.c_in; 

