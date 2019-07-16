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
  a = np.random.random(size) * (maxValue - minValue) + minValue
  a = a.astype(dataType)
  a = np.reshape(a, size)
  return a

class OP_ERROR(Exception):
  def __init__(self, message):
    self.message = message
class OP:
  opDict = {
    'BLAS_L1': ('amax', 'amin', 'asum', 'axpy', 'swap', 'scal', 'dot', 'nrm2'),
    'BLAS_L2': ('gemv', 'gbmv', 'sbmv', 'symv')
  }
  @staticmethod
  def parse(opName):
    for cls, ops in OP.opDict.items():
      if opName in ops:
        return cls
    raise OP_ERROR("opName %s is not supported."%opName)

  def __init__(self):
    pass

  def setDtype(self, dataType):
    self.dataType = dataType


class BLAS_L1(OP):
  @staticmethod
  def parse(opName, maxV, minV):
    try:
      op= eval(opName)(BLAS_L1(opName, maxV, minV))
      return op
    except:
      raise OP_ERROR("%s is not defined."%opName)
  def __init__(self, name, maxV, minV):
    self.name = name
    self.maxV = maxV
    self.minV = minV
    self.interfaceList=('p_x', 'p_y', 'p_xRes', 'p_yRes')
  def copyConstructor(self, object):
    self.name = object.name
    self.maxV = object.maxV
    self.minV = object.minV
    self.interfaceList= object.interfaceList
  def setRtype(self, rtype):
    self.rtype = rtype
  def setSize(self, size):
    self.vectorDim = size
    self.sizeStr = "v%d"%size

  def compute(self): 
    x = y = xr = yr = None
    alpha = dataGen(self.dataType, 1, self.maxV, self.minV)
    r = np.zeros(1)
    return alpha, x, y, xr, yr, r
  def addInstr(self, blas_gen, register):
    for alpha, x, y, xr, yr, r in register:
      blas_gen.addB1Instr(self.name, self.vectorDim, alpha, x, y, xr, yr, r.astype(self.rtype))


  def test(self, runTest):
    vecDimList = runTest.profile['vectorDims']
    dataTypeList = runTest.dataTypes
    retTypeList = runTest.retTypes
    for dataType, retType in zip(dataTypeList, retTypeList):
      self.setDtype(dataType)
      self.setRtype(retType)
      for vecDim in vecDimList:
        self.setSize(vecDim)
        runTest.run()

  def paramTCL(self, f):
    f.write('   L1 true\n ')
    f.write('   opName "%s"\n '%self.name)
    f.write('   vectorSize %d\n '%self.vectorDim)

class amin(BLAS_L1):
  def __init__(self, blas_l1: BLAS_L1):
    self.copyConstructor(blas_l1)

  def compute(self): 
    alpha, x, y, xr, yr, r = BLAS_L1.compute(self)
    x = dataGen(self.dataType, self.vectorDim, self.maxV, self.minV)
    r = np.argmin(np.abs(x))
    return alpha, x, y, xr, yr, r


class amax(BLAS_L1):
  def __init__(self, blas_l1: BLAS_L1):
    self.copyConstructor(blas_l1)

  def compute(self): 
    alpha, x, y, xr, yr, r = BLAS_L1.compute(self)
    x = dataGen(self.dataType, self.vectorDim, self.maxV, self.minV)
    r = np.argmax(np.abs(x))
    return alpha, x, y, xr, yr, r

class asum(BLAS_L1):
  def __init__(self, blas_l1: BLAS_L1):
    self.copyConstructor(blas_l1)

  def compute(self): 
    alpha, x, y, xr, yr, r = BLAS_L1.compute(self)
    x = dataGen(self.dataType, self.vectorDim, self.maxV, self.minV)
    r = np.sum(np.abs(x))
    return alpha, x, y, xr, yr, r

class axpy(BLAS_L1):
  def __init__(self, blas_l1: BLAS_L1):
    self.copyConstructor(blas_l1)

  def compute(self): 
    alpha, x, y, xr, yr, r = BLAS_L1.compute(self)
    x = dataGen(self.dataType, self.vectorDim, self.maxV, self.minV)
    y = dataGen(self.dataType, self.vectorDim, self.maxV, self.minV)
    yr = alpha * x+ y
    return alpha, x, y, xr, yr, r

class copy(BLAS_L1):
  def __init__(self, blas_l1: BLAS_L1):
    self.copyConstructor(blas_l1)

  def compute(self): 
    alpha, x, y, xr, yr, r = BLAS_L1.compute(self)
    yr = x = dataGen(self.dataType, self.vectorDim, self.maxV, self.minV)
    return alpha, x, y, xr, yr, r

class dot(BLAS_L1):
  def __init__(self, blas_l1: BLAS_L1):
    self.copyConstructor(blas_l1)

  def compute(self): 
    alpha, x, y, xr, yr, r = BLAS_L1.compute(self)
    x = dataGen(self.dataType, self.vectorDim, self.maxV, self.minV)
    y = dataGen(self.dataType, self.vectorDim, self.maxV, self.minV)
    r=np.dot(x, y)
    return alpha, x, y, xr, yr, r

class nrm2(BLAS_L1):
  def __init__(self, blas_l1: BLAS_L1):
    self.copyConstructor(blas_l1)

  def compute(self): 
    alpha, x, y, xr, yr, r = BLAS_L1.compute(self)
    x = dataGen(self.dataType, self.vectorDim, self.maxV, self.minV)
    r = np.linalg.norm(x)
    return alpha, x, y, xr, yr, r

class swap(BLAS_L1):
  def __init__(self, blas_l1: BLAS_L1):
    self.copyConstructor(blas_l1)

  def compute(self): 
    alpha, x, y, xr, yr, r = BLAS_L1.compute(self)
    yr = x = dataGen(self.dataType, self.vectorDim, self.maxV, self.minV)
    xr = y = dataGen(self.dataType, self.vectorDim, self.maxV, self.minV)
    return alpha, x, y, xr, yr, r

class scal(BLAS_L1):
  def __init__(self, blas_l1: BLAS_L1):
    self.copyConstructor(blas_l1)

  def compute(self): 
    alpha, x, y, xr, yr, r = BLAS_L1.compute(self)
    x = dataGen(self.dataType, self.vectorDim, self.maxV, self.minV)
    xr = alpha * x
    return alpha, x, y, xr, yr, r


class BLAS_L2(OP):
  @staticmethod
  def parse(opName, maxV, minV):
    try:
      op = eval(opName)(BLAS_L2(opName, maxV, minV))
      return op
    except:
      raise OP_ERROR("%s is not defined."%opName)

  def __init__(self, name, maxV, minV):
    self.name=name
    self.maxV = maxV
    self.minV = minV
    self.ku=0
    self.kl=0
    self.m=0
    self.n=0
    self.interfaceList=('p_a', 'p_x', 'p_y', 'p_aRes', 'p_yRes')

  def copyConstructor(self, object):
    self.name = object.name
    self.maxV = object.maxV
    self.minV = object.minV
    self.ku = object.ku
    self.kl=object.kl
    self.m=object.m
    self.n=object.n
    self.interfaceList= object.interfaceList

  def compute(self): 
    a = x = y = ar = yr = None
    alpha = beta = dataGen(self.dataType, 1, self.minV, self.maxV)
    beta = beta = dataGen(self.dataType, 1, self.minV, self.maxV)
    return alpha, beta, a, x, y, ar, yr

  def addInstr(self, blas_gen, register):
    for alpha, beta, a, x, y, ar, yr in register:
      blas_gen.addB2Instr(self.name, self.m, self.n, self.kl, self.ku, 
        alpha, beta, a, x, y, ar, yr)

  def setSize(self, mn):
    self.matrixDim = tuple(mn)
    self.m = mn[0]
    self.n = mn[1]
    self.sizeStr = "m%d-n%d"%(self.m,self.n)

  def paramTCL(self, f):
    f.write('   L2 true\n ')
    f.write('   opName "%s"\n '%self.name)
    f.write('   matrixSize %d\n '%(self.m * self.n))
    f.write('   vectorSize %d\n '%self.n)

  def test(self, runTest):
    vecDimList = runTest.profile['matrixDims']
    dataTypeList = runTest.dataTypes
    for dataType in dataTypeList:
      self.setDtype(dataType)
      for vecDim in vecDimList:
        self.setSize(vecDim)
        runTest.run()

class gemv(BLAS_L2):
  def __init__(self, blas_l2: BLAS_L2):
    self.copyConstructor(blas_l2)
  def compute(self):
    alpha, beta, a, x, y, ar, yr = BLAS_L2.compute(self)
    a = dataGen(self.dataType, self.matrixDim, self.maxV, self.minV)
    x = dataGen(self.dataType, self.n, self.maxV, self.minV)
    y = dataGen(self.dataType, self.m, self.maxV, self.minV)
    yr = alpha * np.matmul(a, x) + beta * y
    return alpha, beta, a, x, y, ar, yr

class gbmv(BLAS_L2):
  def __init__(self, blas_l2: BLAS_L2):
    self.copyConstructor(blas_l2)

  def setKul(self, ku, kl):
    self.ku = ku
    self.kl = kl

  def compute(self):
    alpha, beta, a, x, y, ar, yr = BLAS_L2.compute(self)
    a = np.zeros((self.ku+self.kl+1, self.m), dtype=self.dataType)
    matrix = dataGen(self.dataType, self.matrixDim, self.maxV, self.minV)
    v = np.diag(matrix)
    a[self.ku][:] = v
    band_mat = np.diag(v)
    for i in range(self.ku, 0, -1):
      v = np.diag(matrix, i)
      a[self.ku - i][self.m-len(v):] = v
      band_mat = band_mat + np.diag(v, i)
    for i in range(-self.kl, 0):
      v = np.diag(matrix, i)
      a[self.ku-i][0:len(v)] = v
      band_mat = band_mat + np.diag(v, i)
    x = dataGen(self.dataType, self.n, self.maxV, self.minV)
    y = dataGen(self.dataType, self.m, self.maxV, self.minV)
    yr = alpha * np.matmul(band_mat, x) + beta * y
    return alpha, beta, a, x, y, ar, yr

  def test(self, runTest):
    matrixDimList = runTest.profile['matrixDims']
    dataTypeList = runTest.dataTypes
    kulList = runTest.profile['kulList']
    for dataType in dataTypeList:
      self.setDtype(dataType)
      for kul in kulList:
        self.setKul(kul)
        self.sizeStr = "m%d-n%d-u%d-l%d"%(self.m,self.n,self.ku,self.kl)
        for matrixDim in matrixDimList:
          self.setSize(matrixDim)
          runTest.run()

class symv(BLAS_L2):
  def __init__(self, blas_l2: BLAS_L2):
    self.copyConstructor(blas_l2)

  def setSize(self, m):
    self.matrixDim =(m ,m) 
    self.m = m
    self.sizeStr = "m%d"%(self.m)

  def compute(self):
    alpha, beta, a, x, y, ar, yr = BLAS_L2.compute(self)
    a = dataGen(self.dataType, self.matrixDim, self.maxV, self.minV)
    a = a + np.transpose(a)
    x = dataGen(self.dataType, self.m, self.maxV, self.minV)
    y = dataGen(self.dataType, self.m, self.maxV, self.minV)
    yr = alpha * np.matmul(a, x) + beta * y
    return alpha, beta, a, x, y, ar, yr

class sbmv(BLAS_L2):
  def __init__(self, blas_l2: BLAS_L2):
    self.copyConstructor(blas_l2)

  def setK(self, k):
    self.k = k

  def setSize(self, m):
    self.matrixDim =(m ,m) 
    self.m = m
    self.sizeStr = "m%d"%(self.m)

  def compute(self):
    alpha, beta, a, x, y, ar, yr = BLAS_L2.compute(self)
    a = np.zeros((self.k*2+1, self.m), dtype=self.dataType)
    v = dataGen(self.dataType, self.m, self.maxV, self.minV)
    a[self.k][:] = v
    band_mat = np.diag(v)
    for i in range(1, self.k+1):
      v = dataGen(self.dataType, self.m - i, self.maxV, self.minV)
      a[self.k + i][:len(v)] = v
      a[self.k - i][self.m-len(v):] = v
      band_mat = band_mat + np.diag(v, i)
      band_mat = band_mat + np.diag(v, -i)
    x = dataGen(self.dataType, self.m, self.maxV, self.minV)
    y = dataGen(self.dataType, self.m, self.maxV, self.minV)
    yr = alpha * np.matmul(band_mat, x) + beta * y
    return alpha, beta, a, x, y, ar, yr

  def test(self, runTest):
    matrixDimList = runTest.profile['matrixDims']
    dataTypeList = runTest.dataTypes
    kulList = runTest.profile['kulList']
    for dataType in dataTypeList:
      self.setDtype(dataType)
      for k in kList:
        self.setK(k)
        self.sizeStr = "m%d-k%d"%(self.m,self.k)
        for matrixDim in matrixDimList:
          self.setSize(matrixDim)
          runTest.run()


def main():

  opName = 'sbmv'
  className = None
  for cls, ops in OP.opDict.items():
    if opName in ops:
      className = cls
      break

  if className:
    op  = eval(className).parse(opName, -24,   24)
    op.setDtype(np.int32)
    op.setSize(6)
    op.setK(2)
    alpha, beta, a, x, y, ar, yr = op.compute()

if __name__=='__main__':
  main()
      
  
