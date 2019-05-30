import numpy as np
import argparse
import shlex, subprocess
import os

def main(b_csim, b_synth, b_cosim, b_max, p_vectorSize, p_parEntries):
  a=(np.random.rand(p_vectorSize) - 0.5) * 1e5

  vectorPath = r"./amaxmin/data/vector_%d.csv"%p_vectorSize
  tclPath = r"./amaxmin/build/run.tcl"
  np.savetxt(vectorPath, a , fmt='%.8f', delimiter=',')
  a=np.abs(a)
  amax = np.argmax(a)
  amin = np.argmin(a)

  commandLine ='''vivado_hls -f %s "runCsim %d runRTLsynth %d \
runRTLsim %d part vu9p op amin dataType double dataWidth 64 indexType int size %d \
entriesInParallel %d runArgs \
'%s %d %d'"'''%(tclPath, b_csim, b_synth, b_cosim, p_vectorSize, p_parEntries,
		   os.path.abspath(vectorPath), p_vectorSize, amin)
  args = shlex.split(commandLine)
  subprocess.call(args)
  
  commandLine ='''vivado_hls -f %s "runCsim %d runRTLsynth %d \
runRTLsim %d part vu9p op amax dataType double dataWidth 64 indexType int size %d \
entriesInParallel %d runArgs \
'%s %d %d'"'''%(tclPath, b_csim, b_synth, b_cosim, p_vectorSize, p_parEntries,
		   os.path.abspath(vectorPath), p_vectorSize, amax)
  args = shlex.split(commandLine)
  subprocess.call(args)
  
if __name__== "__main__":
  parser = argparse.ArgumentParser(description='Generate random vectors and \
      run test.')
  parser.add_argument('size', type=int, metavar='N', help='size of the \
      vector')
  parser.add_argument('-p', type=int, help='Number of parallel entries')
  args = parser.parse_args()
  main(1, 1, 1 ,False, args.size, args.p)
