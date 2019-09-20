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
from operation import *
from time import sleep

typeDict ={
      np.int16: 'short',
      np.float32: 'float',
      np.float64: 'double'
    }

class RunTest:
  def  __init__(self):
    self.profile = None
    
  def parseProfile(self, filePath, shell):
    with open(filePath, 'r') as fh:
      self.profile = json.loads(fh.read())
    self.opName = self.profile['op']
    self.dataTypes = [eval('np.%s'%dt) for dt in self.profile['dataTypes']]
    self.cppDataTypes = [typeDict[dt] for dt in self.dataTypes]
    self.minValue = self.profile['valueRange'][0]
    self.maxValue = self.profile['valueRange'][1]
    self.dimList = self.profile['matrixDims']
    self.shell = shell
    
  def build(self): 
    for dataType in self.cppDataTypes:
      commandLine = r'make host OPERATOR_NAME=%s XFBLAS_dataType=%s'%(self.opName,dataType)
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
      logFile = open(r'out_test/%s/log_%s.txt'%(self.opName,dataType),"w") 
      for dim in self.dimList:
        commandLine = r'out_test/%s/test_%s.exe ../overlay/%s/%s_%s_1kernel/gemx.xclbin ../overlay/%s/%s_%s_1kernel/config_info.dat %d out_test/%s/data/%s/'%(self.opName, dataType, self.shell, self.opName, dataType, self.shell, self.opName, dataType, i, self.opName,dataType)
        print("**************** Running Command ****************")
        print(commandLine)
        args = shlex.split(commandLine)
        result = subprocess.check_output(args).decode("utf-8")
        print(result)
        logFile.write(result)
        i = i + 1
      logFile.close()
        
def main(profileList, shell):
  commandLine = 'make clean'
  args = shlex.split(commandLine)
  subprocess.call(args)
  for profile in profileList:
    runTest = RunTest()
    runTest.parseProfile(profile,shell)
    runTest.build()
    sleep(0.1)
    runTest.genBin()
    sleep(0.1)
    runTest.run()
  print("******************* TEST DONE *******************")
  print("See compare report in out_test/OPERATOR_NAME/log_DATATYPE.txt")
  print("See runtime report in out_test/xrt_report.txt")


if __name__== "__main__":
  parser = argparse.ArgumentParser(description='Generate random matrices and run test.')
  group = parser.add_mutually_exclusive_group(required=True)
  group.add_argument('--profile', nargs='*', metavar='profile.json', help='list of pathes to the profile files')
  group.add_argument('--operator', nargs='*',metavar='opName', help='list of operator names')
  parser.add_argument('--shell',default='vcu1525_dynamic_5_1', help='choice of xclbin')
  args = parser.parse_args()
  
  profile = list()
  if args.profile:
    profile = args.profile
  elif args.operator:
    for op in args.operator:
      profile.append('./xf_blas/%s/profile.json'%op)
  else:
    parser.print_help()
    
  main(set(profile), args.shell)
