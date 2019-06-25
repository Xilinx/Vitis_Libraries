import shlex, subprocess

class HLS:
  def __init__(self, tclPath, b_csim, b_syn, b_cosim):

    self.tcl = tclPath
    self.csim = b_csim
    self.syn = b_syn 
    self.cosim = b_cosim 

  def execution(self, opArgs, runArgs):
    commandLine ='''vivado_hls -f %s "runCsim %d runRTLsynth %d \
runRTLsim %d part vu9p %s runArgs '%s'"'''%(self.tcl, self.csim, self.syn,
  self.cosim, opArgs, runArgs)
    args = shlex.split(commandLine)
    exitCode = subprocess.call(args)
    return exitCode

