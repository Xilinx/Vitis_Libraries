import ctypes as C
import numpy as np
import argparse
import shlex, subprocess
import os, sys
import json

class BLAS_GEN:
  def __init__(self, lib):
    self.lib = lib
    self.lib.genBinNew.restype=C.c_void_p
    self.obj = lib.genBinNew()
  def addB1Instr(self, p_opName, p_n, p_alpha, p_x, p_y, p_xRes, p_yRes, p_res):
    func=self.lib.addB1Instr
    func.argtypes=[C.c_void_p, C.c_char_p, C.c_int, C.c_int,
      C.c_void_p,C.c_void_p,C.c_void_p,C.c_void_p,
      C.c_int]
    return func(self.obj, p_opName.encode('utf-8'), p_n, p_alpha, p_x, p_y, p_xRes, p_yRes, p_res)
  def write2BinFile(self, p_fileName):
    func=self.lib.write2BinFile
    func.argtypes=[C.c_void_p, C.c_char_p]
    return func(self.obj, p_fileName.encode('utf-8'))

  def readFromBinFile(self, p_fileName):
    func=self.lib.readFromBinFile
    func.argtypes=[C.c_void_p, C.c_char_p]
    return func(self.obj, p_fileName.encode('utf-8'))

  def printProgram(self):
    func=self.lib.printProgram 
    func.argtypes=[C.c_void_p]
    func(self.obj)

  def __del__(self):
    func=self.lib.genBinDel
    func.argtypes=[C.c_void_p]
    func(self.obj)




def main(lib, path):
  if path == None:
    path = './'
  blas_gen=BLAS_GEN(lib)
  blas_read=BLAS_GEN(lib)
  filename = r'app.bin'
  filepath = os.path.join(path, filename)
  size = 16
  x = np.ctypeslib.as_ctypes((np.linspace(1, size, size, dtype=np.int32)))
  ptr_x  = C.pointer(x)
  blas_gen.addB1Instr('amin', size, 0, ptr_x, None, None, None, 1)
  blas_gen.write2BinFile(filepath)
  print("write file sucessfully.")
  blas_read.readFromBinFile(filepath)
  blas_read.printProgram()

if __name__=="__main__":
  parser = argparse.ArgumentParser(description='Run HLS test.')
  parser.add_argument('so', type=str, metavar='sharedLibrary', help='path to the shared library file')
  parser.add_argument('-p', type=str, metavar='outputPath', help='path to generate bin files')
  args = parser.parse_args()
  lib = C.cdll.LoadLibrary(args.so)
  main(lib, args.p)

