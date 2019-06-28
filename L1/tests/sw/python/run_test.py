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
import ctypes as C
import argparse
import os, sys
import json
from blas_gen_bin import BLAS_GEN
from hls import HLS
from makefile import Makefile
import pdb
from operation import BLAS_L1

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


    self.logParEntries = self.profile['logParEntries']
    
    self.minValue = self.profile['valueRange'][0]
    self.maxValue = self.profile['valueRange'][1]

    self.numSim = self.profile['numSimulation']

    self.minSize = self.profile['vectorSizes'][0]
    self.maxSize = self.profile['vectorSizes'][1]

    self.hls = HLS(r'build/run-hls.tcl', self.profile['b_csim'],
        self.profile['b_synth'], self.profile['b_cosim'])
    self.datapath = r'out_test/data' #self.profile['dataPath']
    if not os.path.exists(self.datapath):
      os.mkdir(self.datapath)

  def runTest(self, libpath, makefile):
    make = Makefile(makefile, libpath)
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
      if  make.make(c_type, r_type)!= 0:
        print("ERROR: make shared library failure.")
        sys.exit
      lib = C.cdll.LoadLibrary(libpath)
      for j in range(self.numSim): 
        #pdb.set_trace()
        vectorSize = (np.random.randint(self.minSize, self.maxSize)>> self.logParEntries)<< self.logParEntries
        op = BLAS_L1.parse(self.op,dtype, vectorSize, self.maxValue, self.minValue) 
        alpha, xdata, ydata, xr, yr, r = op.compute()
        binFile =os.path.join(self.datapath,
          'TestBin_v%d_d%s%s_r%s%d.bin'%(vectorSize,dt,dw,rt,rw))
        blas_gen=BLAS_GEN(lib)
        blas_gen.addB1Instr(self.op, vectorSize, alpha, xdata, ydata, xr, yr,
            r.astype(rtype))
        blas_gen.write2BinFile(binFile)
        print("write file sucessfully.")
        blas_read=BLAS_GEN(lib)
        blas_read.readFromBinFile(binFile)
        blas_read.printProgram()
        
        opArgs=r'op %s dataType %s dataWidth %d resDataType %s logParEntries %d'%(self.op, c_type, dw, r_type, self.logParEntries)
        runArgs=os.path.abspath(binFile)
        result = self.hls.execution(opArgs, runArgs)

  def generateTCL(self):
    pass
                  

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
