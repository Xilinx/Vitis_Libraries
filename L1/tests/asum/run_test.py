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
  a=(np.random.rand(profile['vectorSize']) - 0.5) 
  vectorPath = os.path.join(testPath, profile['dataPath'], r"vector_%d.csv"%profile['vectorSize'])
  if profile['dataType'] == 'double' or profile['dataType'] == 'float': 
    np.savetxt(vectorPath, a , fmt='%.8f', delimiter=',')
  else:
    a = a.astype(int) 
    np.savetxt(vectorPath, a , fmt='%d', delimiter=',')
  a=np.abs(a)
  result=np.sum(a)
  commandLine ='''vivado_hls -f %s "runCsim %d runRTLsynth %d \
runRTLsim %d part vu9p dataType %s dataWidth %d indexType int size %d \
entriesInParallel %d runArgs \
'%s %d %f'"'''%(os.path.join(testPath, profile['tclPath']), 
    profile['b_csim'], profile['b_synth'],
    profile['b_cosim'], profile['dataType'],
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
