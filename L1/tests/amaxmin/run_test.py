import numpy as np
import argparse
import shlex, subprocess
import os, sys
from dataProc import loadProfile
import json

def main(testPath, profile):
  a=(np.random.rand(profile['vectorSize']) - 0.5) * profile['vectorSize']
  vectorPath = os.path.join(testPath, profile['dataPath'], r"vector_%d.csv"%profile['vectorSize'])
  if profile['op'] == 'double' or profile['op'] == 'float': 
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
    print('Operation is not supported')
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
  subprocess.call(args)
  
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
