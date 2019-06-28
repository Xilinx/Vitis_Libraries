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
import argparse
import shlex, subprocess
import os, sys
import json

def loadProfile(filePath):
  with open(filePath, 'r') as fh:
    profile = json.loads(fh.read())
    return profile


def writeProfile(profile, filePath):
  with open(filePath, 'w') as fh:
    fh.write(json.dumps(profile, indent=2))
 

def main(testPath, profile):
  a=(np.random.rand(profile['vectorSize']) - 0.5) * profile['vectorSize']
  vectorPath = os.path.join(testPath, profile['dataPath'], r"vector_%d.csv"%profile['vectorSize'])
  if profile['dataType'] == 'double' or profile['dataType'] == 'float': 
    np.savetxt(vectorPath, a , fmt='%.8f', delimiter=',')
  else:
    a = a.astype(int) 
    np.savetxt(vectorPath, a , fmt='%d', delimiter=',')
  a=np.abs(a)
  if profile['op'] == 'amax':
    result = np.argmax(a)
  elif profile['op'] == 'amin':
    result = np.argmin(a)
  else:
    print('ERROR: Operation is not supported')
    sys.exit

  commandLine ='''vivado_hls -f %s "runCsim %d runRTLsynth %d \
runRTLsim %d part vu9p op %s dataType %s dataWidth %d indexType int size %d \
entriesInParallel %d runArgs \
'%s %d %d'"'''%(os.path.join(testPath, profile['tclPath']), 
    profile['b_csim'], profile['b_synth'],
    profile['b_cosim'], profile['op'], profile['dataType'],
    profile['dataWidth'], profile['vectorSize'], profile['parEntries'],
    os.path.abspath(vectorPath), profile['vectorSize'], result)
  args = shlex.split(commandLine)
  exitCode = subprocess.call(args)
  if exitCode == 0:
    print("Test Passed.")
  else:
    print("Test failed, please check log file for more details.")
  
if __name__== "__main__":
  parser = argparse.ArgumentParser(description='Generate random vectors and run test.')
  parser.add_argument('-p', type=str, metavar='Profile', help='path to the profile file')
  args = parser.parse_args()
  profile=dict()
  filePath = os.path.realpath(__file__)
  path = os.path.dirname(filePath)
  if args.p !=None and os.path.isfile(args.p):
    profile = loadProfile(args.p)
    print("profile %s is loaded"%args.p)
  else:
    print("profile file is not provided. The default [%s] will be loaded."%(os.path.join(path,'profile.json')))
    profile = loadProfile(os.path.join(path,'profile.json'))
  main(path, profile)
