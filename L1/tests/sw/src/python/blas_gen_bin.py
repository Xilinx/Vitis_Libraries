import pdb
import ctypes as ct
import numpy as np
import argparse
import os

class BLAS_GEN:
  def __init__(self, lib):
    c_types = [ct.c_byte, ct.c_short, ct.c_int, ct.c_long, ct.c_longlong,
      ct.c_ubyte, ct.c_ushort, ct.c_uint, ct.c_ulong,
      ct.c_ulonglong, ct.c_float, ct.c_double ]
    self.lib = lib
    self.lib.genBinNew.restype=ct.c_void_p
    self.obj = lib.genBinNew()
    self.typeDict = {np.dtype(ctype): ctype for ctype in c_types}

  def _getType(self, x):
    return self.typeDict[x.dtype]


  def _getPointer(self, x):
    try:  
      ptr_x = ct.pointer(np.ctypeslib.as_ctypes(x))
      return ptr_x
    except:
      return None


  def addB1Instr(self, p_opName, p_n, p_alpha, p_x, p_y, p_xRes, p_yRes, p_res):
    func=self.lib.addB1Instr
    func.argtypes=[ct.c_void_p, ct.c_char_p, ct.c_int, 
      self._getType(p_alpha),  ct.c_void_p,ct.c_void_p,ct.c_void_p,
      ct.c_void_p, self._getType(p_res)]
    return func(self.obj, p_opName.encode('utf-8'), p_n, p_alpha,
        self._getPointer(p_x), self._getPointer(p_y), self._getPointer(p_xRes),
            self._getPointer(p_yRes), p_res)

  def write2BinFile(self, p_fileName):
    func=self.lib.write2BinFile
    func.argtypes=[ct.c_void_p, ct.c_char_p]
    return func(self.obj, p_fileName.encode('utf-8'))

  def readFromBinFile(self, p_fileName):
    func=self.lib.readFromBinFile
    func.argtypes=[ct.c_void_p, ct.c_char_p]
    return func(self.obj, p_fileName.encode('utf-8'))

  def printProgram(self):
    func=self.lib.printProgram 
    func.argtypes=[ct.c_void_p]
    func(self.obj)

  def clearProgram(self):
    func=self.lib.clearProgram 
    func.argtypes=[ct.c_void_p]
    func(self.obj)

  def __del__(self):
    func=self.lib.genBinDel
    func.argtypes=[ct.c_void_p]
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
  ptr_x  = ct.pointer(x)
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
  lib = ct.cdll.LoadLibrary(args.so)
  main(lib, args.p)

