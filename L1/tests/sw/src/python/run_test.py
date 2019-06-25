/*
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

*/
import numpy as np
import ctypes as C
import argparse
import os, sys
import json
from blas_gen_bin import BLAS_GEN
from hls import HLS
from makefile import Makefile
import pdb

class RunTest:
  def __init__(self):
    self.profile = None 
    self.opList = None 
    self.dtList =  None
    self.vectorSize = None  
    self.dtWidth = None  
    self.parEntries = None
    self.valueRange = None
    self.numSim = None

    self.hls = None
    self.typeDict ={
      np.int8: 'char',
      np.int16: 'short',
      np.int32: 'int',
      np.int64: 'long long int',
      np.uint8: 'unsigned char',
      np.uint16: 'unsigned short',
      np.uint32: 'unsigned int',
      np.uint64: 'unsigned long long int',
      np.float32: 'float',
      np.float64: 'double'
    }

  def parseProfile(self, filePath):
    with open(filePath, 'r') as fh:
      self.profile = json.loads(fh.read())
    self.op = self.profile['op']
    self.dtList = self.profile['dataTypes']
    self.rtList = self.profile['retTypes']


    self.parEntries = self.profile['parEntries']
    
    self.minValue = self.profile['valueRange'][0]
    self.maxValue = self.profile['valueRange'][1]

    self.numSim = self.profile['numSimulation']

    self.minSize = self.profile['vectorSizes'][0]
    self.maxSize = self.profile['vectorSizes'][1]

    self.parallel = self.profile['parEntries']

    self.hls = HLS(self.profile['tclPath'], self.profile['b_csim'],
        self.profile['b_synth'], self.profile['b_cosim'])
    self.datapath = self.profile['dataPath']
    if not os.path.exists(self.datapath):
      os.mkdir(self.datapath)

  def dataGen(self, size, dataType):
    a = np.random.rand(size) * (self.maxValue -self.minValue) + self.minValue
    a = a.astype(dataType)
    return a


  def compute(self, op, alpha, x, y):
    xr = None
    yr = None
    result = -1
    if op =='amax':
      result = np.argmax(np.abs(x))
    elif op=='amin':
      result = np.argmin(np.abs(x))
    elif op=='asum':
      result = np.sum(np.abs(x))
    elif op=='axpy':
      yr = alpha * x + y
    elif op=='copy':
      result = x
    elif op=='dot':
      result = np.dot(x, y)
    elif op=='scal':
      xr = alpha * x
    elif op=='swap':
      xr, yr = y, x
    else:
      print("ERROR: Operation '%s' is not supported."%op)
      sys.exit
    return xr, yr, result

  def runTest(self, libpath, makefile):
    make = Makefile(makefile)
    dtLen =  len(self.dtList)
    for index in range(dtLen):
      dt = self.dtList[index][0]
      dw = self.dtList[index][1]
      dtype = eval(r'np.%s%d'%(dt, dw))
      rt = self.rtList[index][0]
      rw = self.rtList[index][1]
      rtype = eval(r'np.%s%d'%(rt, rw))

      c_type=self.typeDict[dtype]
      r_type=self.typeDict[rtype]
      if  make.make(libpath, c_type, r_type)!= 0:
        print("ERROR: make shared library failure.")
        sys.exit
      lib = C.cdll.LoadLibrary(libpath)
      for j in range(self.numSim): 
        vectorSize = np.random.randint(self.minSize, self.maxSize)
        xdata = self.dataGen(vectorSize, dtype)
        ydata = self.dataGen(vectorSize, dtype)
        alpha = self.dataGen(1, dtype)
        xr, yr, r = self.compute(self.op, alpha, xdata, ydata)
        binFile =os.path.join(self.datapath,
          'TestBin_v%d_d%s%s_r%s%d.bin'%(vectorSize,dt,dw, rt,rw))
        blas_gen=BLAS_GEN(lib)
        blas_gen.addB1Instr(self.op, vectorSize, alpha, xdata, ydata, xr, yr, r)
        blas_gen.write2BinFile(binFile)
        print("write file sucessfully.")
        blas_read=BLAS_GEN(lib)
        blas_read.readFromBinFile(binFile)
        #pdb.set_trace()
        blas_read.printProgram()

  #     opArgs=r'''op %s dataType %s dataWidth %d indexType int size %d \
  #entriesInParallel %d'''%(op, dt, dtWidth, vectorSize, self.parallel)
  #      runArgs=os.path.abspath(binFile)
  #      result = self.hls.execution(opArgs, runArgs)
                  

def main(lib, profile):
  runTest = RunTest()
  runTest.parseProfile(profile)
  runTest.runTest(lib, 'Makefile')
  
if __name__== "__main__":
  parser = argparse.ArgumentParser(description='Generate random vectors and run test.')
  parser.add_argument('p', type=str, metavar='Profile', help='path to the profile file')
  args = parser.parse_args()
  libpath=r'out_test/blas_gen_wrapper.so'
  main(libpath, args.p)
