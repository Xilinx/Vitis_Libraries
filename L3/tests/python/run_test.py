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
import json
import shlex, subprocess
from operation import gemm, gemv

typeDict ={
      np.int16: 'short',
      np.float32: 'float',
      np.float64: 'double'
    }

class RunTest:
  def  __init__(self):
    self.profile = None
    
  def parseProfile(self, filePath):
    with open(filePath, 'r') as fh:
      self.profile = json.loads(fh.read())
    self.opName = self.profile['op']
    self.dataTypes = [eval('np.%s'%dt) for dt in self.profile['dataTypes']]
    self.cppDataTypes = [typeDict[dt] for dt in self.dataTypes]
    self.minValue = self.profile['valueRange'][0]
    self.maxValue = self.profile['valueRange'][1]
    self.dimList = self.profile['matrixDims']
    
  def build(self): 
    for dataType in self.cppDataTypes:
      commandLine = r'make OPERATOR_NAME=%s XFBLAS_dataType=%s'%(self.opName,dataType)
      args = shlex.split(commandLine)
      subprocess.call(args)
      
  def genBin(self):
    for dataType in self.dataTypes:
      i = 0
      for dim in self.dimList:
        if self.opName == 'gemm':
          gemm().genBin(i, dataType, typeDict[dataType], dim, self.maxValue, self.minValue)
        elif self.opName == 'gemv':
          gemv().genBin(i, dataType, typeDict[dataType], dim, self.maxValue, self.minValue)
        else:
          print ('op is not supported yet')
        i = i + 1

  def run(self):
    for dataType in self.cppDataTypes:
      i = 0
      for dim in self.dimList:
        commandLine = r'out_test/%s/test_%s.exe gemx.xclbin config_info.dat %d out_test/%s/data/%s/'%(self.opName, dataType, i, self.opName,dataType)
        args = shlex.split(commandLine)
        subprocess.call(args)
        i = i + 1
        
def main(profileList):
  commandLine = 'make clean'
  args = shlex.split(commandLine)
  subprocess.call(args)
  for profile in profileList:
    runTest = RunTest()
    runTest.parseProfile(profile)
    runTest.build()
    runTest.genBin()
    runTest.run()


if __name__== "__main__":
  parser = argparse.ArgumentParser(description='Generate random matrices and run test.')
  group = parser.add_mutually_exclusive_group(required=True)
  group.add_argument('--profile', nargs='*', metavar='profile.json', help='list of pathes to the profile files')
  group.add_argument('--operator', nargs='*',metavar='opName', help='list of operator names')
  args = parser.parse_args()
  
  profile = list()
  if args.profile:
    profile = args.profile
  elif args.operator:
    for op in args.operator:
      profile.append('./xf_blas/%s/profile.json'%op)
  else:
    parser.print_help()
    
  main(set(profile))
