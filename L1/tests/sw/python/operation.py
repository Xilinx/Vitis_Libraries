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
import pdb


def dataGen(dataType, size, maxValue, minValue):
  a = np.random.rand(size) * (maxValue - minValue) + minValue
  a = a.astype(dataType)
  return a


class OP:
  def __init__(self):
    self.name = str()

class BLAS_L1(OP):
  def __init__(self, name, dtype, dsize, maxV, minV):
    self.name=name
    self.dtype = dtype
    self.dsize = dsize
    self.maxV = maxV
    self.minV = minV
  def compute(self): 
    x = y = xr = yr = None
    alpha = dataGen(self.dtype, 1, 0, 0)
    r = np.zeros(1)
    return alpha, x, y, xr, yr, r

  @staticmethod
  def parse(op, dtype, dsize, maxV, minV):
    try:
      return eval(op)(dtype, dsize, maxV, minV)
    except:
      print("%s is not defined."%op)
      return BLAS_L1(dtype, dsize, maxV, minV)

class amin(BLAS_L1):
  def __init__(self, dtype, dsize, maxV, minV):
    BLAS_L1.__init__(self, 'amin', dtype, dsize, maxV, minV)
  def compute(self): 
    alpha, x, y, xr, yr, r = BLAS_L1.compute(self)
    x = dataGen(self.dtype, self.dsize, self.maxV, self.minV)
    r = np.argmin(np.abs(x))
    return alpha, x, y, xr, yr, r


class amax(BLAS_L1):
  def __init__(self, dtype, dsize, maxV, minV):
    BLAS_L1.__init__(self, 'amax', dtype, dsize, maxV, minV)
  def compute(self): 
    alpha, x, y, xr, yr, r = BLAS_L1.compute(self)
    x = dataGen(self.dtype, self.dsize, self.maxV, self.minV)
    r = np.argmax(np.abs(x))
    return alpha, x, y, xr, yr, r

class asum(BLAS_L1):
  def __init__(self, dtype, dsize, maxV, minV):
    BLAS_L1.__init__(self, 'asum', dtype, dsize, maxV, minV)
  def compute(self): 
    alpha, x, y, xr, yr, r = BLAS_L1.compute(self)
    x = dataGen(self.dtype, self.dsize, self.maxV, self.minV)
    r = np.sum(np.abs(x))
    return alpha, x, y, xr, yr, r

class axpy(BLAS_L1):
  def __init__(self, dtype, dsize, maxV, minV):
    BLAS_L1.__init__(self, 'axpy', dtype, dsize, maxV, minV)
  def compute(self): 
    alpha, x, y, xr, yr, r = BLAS_L1.compute(self)
    x = dataGen(self.dtype, self.dsize, self.maxV, self.minV)
    y = dataGen(self.dtype, self.dsize, self.maxV, self.minV)
    yr = alpha * x+ y
    return alpha, x, y, xr, yr, r

class copy(BLAS_L1):
  def __init__(self, dtype, dsize, maxV, minV):
    BLAS_L1.__init__(self, 'copy', dtype, dsize, maxV, minV)
  def compute(self): 
    alpha, x, y, xr, yr, r = BLAS_L1.compute(self)
    yr = x = dataGen(self.dtype, self.dsize, self.maxV, self.minV)
    return alpha, x, y, xr, yr, r

class dot(BLAS_L1):
  def __init__(self, dtype, dsize, maxV, minV):
    BLAS_L1.__init__(self, 'dot', dtype, dsize, maxV, minV)
  def compute(self): 
    alpha, x, y, xr, yr, r = BLAS_L1.compute(self)
    x = dataGen(self.dtype, self.dsize, self.maxV, self.minV)
    r=np.dot(x, y)
    return alpha, x, y, xr, yr, r

class swap(BLAS_L1):
  def __init__(self, dtype, dsize, maxV, minV):
    BLAS_L1.__init__(self, 'swap', dtype, dsize, maxV, minV)
  def compute(self): 
    alpha, x, y, xr, yr, r = BLAS_L1.compute(self)
    yr = x = dataGen(self.dtype, self.dsize, self.maxV, self.minV)
    xr = y = dataGen(self.dtype, self.dsize, self.maxV, self.minV)
    return alpha, x, y, xr, yr, r

class scal(BLAS_L1):
  def __init__(self, dtype, dsize, maxV, minV):
    BLAS_L1.__init__(self, 'scal', dtype, dsize, maxV, minV)
  def compute(self): 
    alpha, x, y, xr, yr, r = BLAS_L1.compute(self)
    x = dataGen(self.dtype, self.dsize, self.maxV, self.minV)
    xr = alpha * x
    return alpha, x, y, xr, yr, r


def main():

  #pdb.set_trace()
  op  = BLAS_L1.parse('amax', np.uint8, 128, -1024,   1024)
  alpha, x, y, xr, yr, r = op.compute()
  print(x)
  print(r)

if __name__=='__main__':
  main()
      
  
