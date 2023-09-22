# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2023, Advanced Micro Devices, Inc.

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#     http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import sys
import os
import numpy as np
import array as arr

matDim_A = int(sys.argv[1])
matDim_AB = int(sys.argv[2])
matDim_B = int(sys.argv[3])
inp_type = int(sys.argv[4])
iterCnt = int(sys.argv[5])
pliowidth = 128
dataSamps = int(pliowidth / 64)

def printMat(mat,rows,cols,dataSamp,file_name,base_row=0,base_col=0,perl=0):
   dataCtr = 0
   fptr = open(file_name, "a+")
   for row in range(base_row,(base_row + rows)):
      for col in range(base_col,(base_col + cols)):
         if dataCtr < (dataSamp - 1):
            if perl == 1:
               fptr.write(str(mat[row][col].real) + " " + str(mat[row][col].imag) + " ")
            else:
               fptr.write(str(mat[row][col].real) + " " + str(mat[row][col].imag) + " ")
            dataCtr = dataCtr + 1
         elif dataCtr == (dataSamp - 1):
            if perl == 1:
               fptr.write(str(mat[row][col].real) + " " + str(mat[row][col].imag) + "\n")
            else:
               fptr.write(str(mat[row][col].real) + " " + str(mat[row][col].imag) + "\n")
            dataCtr = 0
   fptr.close()

def aiesimInp_gen(dataSamp,inpFile_name,outFile_name,iters):
   inpFptr = open(inpFile_name, "r")
   outFptr = open(outFile_name, "+a")
   dataCtr = 0
   for itr in range(iters):
      for line in inpFptr:
         if dataCtr < (dataSamp - 1):
            outFptr.write(line.strip() + " ")
            dataCtr = dataCtr + 1
         elif dataCtr == (dataSamp - 1):
            outFptr.write(line.strip() + "\n")
            dataCtr = 0
      inpFptr.seek(0)
   inpFptr.close()
   outFptr.close()

def merge_files(inpFile_name1, inpFile_name2, outFile_name, iters):
   inpFptr1 = open(inpFile_name1, "r")
   inpFptr2 = open(inpFile_name2, "r")
   outFptr = open(outFile_name, "+a")
   for itr in range(iters):
      for i in inpFptr1:
         outFptr.write(i)
      for i in inpFptr2:
         outFptr.write(i)
   inpFptr1.close()
   inpFptr2.close()
   outFptr.close()

# Creating Mat A, B and corresponding C...
## Constant Input...
if inp_type == 0:
   matA = np.full((matDim_A,matDim_AB), 1, dtype=complex)
   matB = np.full((matDim_AB,matDim_B), 2, dtype=complex)

## Random Matrix A and B...
else:
   matA = np.random.random((matDim_A, matDim_AB)) + np.random.random((matDim_A, matDim_AB)) * 1j
   matB = np.random.random((matDim_AB, matDim_B)) + np.random.random((matDim_AB, matDim_B)) * 1j

matC = np.matmul(matA, matB)

# Defining splits and cascs based on graph...
splits = 16
cascs = 4

# Creating IO Directories...
ioDir = "gemm_" + str(matDim_A) + "x" + str(matDim_AB) + "x" + str(matDim_B) + "_ioFiles"
os.system("mkdir " + ioDir)

# MatA IO Files Creation...
inpA_python = ioDir + "/a_python.txt"
printMat(matA,matDim_A,matDim_AB,matDim_AB,inpA_python)
inpA_python_serial = ioDir + "/a_python_serial.txt"
printMat(matA,matDim_A,matDim_AB,1,inpA_python_serial,0,0,1)
os.chdir(ioDir)
os.system("perl $DSPLIB_ROOT/L2/tests/aie/common/scripts/matrix_mult_partition_shuffle.pl -f a_python_serial.txt -r " + str(matDim_A) + " -c " + str(matDim_AB) + " -T_DATA_A cfloat -T_DATA_B cfloat -p 4")
for casc in range(cascs):
   aiesimInp_gen(int(dataSamps / 2),"tiled_a_python_serial_" + str(casc) + ".txt","a0_casc" + str(casc) + ".txt",iterCnt * int(splits / 8))
os.chdir("../")

# MatB IO Files Creation...
inpB_python = ioDir + "/b_python.txt"
printMat(matB,matDim_AB,matDim_B,matDim_B,inpB_python)
inpB_python_serial = ioDir + "/b_python_serial.txt"
printMat(matB,matDim_AB,matDim_B,2,inpB_python_serial)
os.chdir(ioDir)
base = 0
for split in range(splits):
   printMat(matB,matDim_AB,int(matDim_B / splits),1,"b" + str(split) + ".txt",0,base,1)
   base = base + int(matDim_B / splits)
   os.system("perl $DSPLIB_ROOT/L2/tests/aie/common/scripts/matrix_mult_partition_shuffle.pl -f b" + str(split) + ".txt -r " + str(matDim_AB) + " -c " + str(int(matDim_B / splits)) + " -T_DATA_A cfloat -T_DATA_B cfloat -p 4 --splitRows")
   for casc in range(cascs):
      aiesimInp_gen(int(dataSamps / 2),"tiled_b" + str(split) + "_" + str(casc) + ".txt","b" + str(split) + "_casc" + str(casc) + "_tmp.txt",int(iterCnt * splits))
for casc in range(cascs):
   for split in range(8):
      merge_files("b" + str(split) + "_casc" + str(casc) + "_tmp.txt", "b" + str(split + 8) + "_casc" + str(casc) + "_tmp.txt", "b" + str(split) + "_casc" + str(casc) + ".txt", iterCnt)

os.chdir("../")

# MatC IO Files Creation...
outC_python = ioDir + "/c_python.txt"
printMat(matC,matDim_A,matDim_B,matDim_B,outC_python)
outC_python_serial = ioDir + "/c_python_serial.txt"
printMat(matC,matDim_A,matDim_B,2,outC_python_serial,0,0,1)
os.chdir(ioDir)
base = 0
for split in range(splits):
   printMat(matC,matDim_A,int(matDim_B / splits),1,"c" + str(split) + ".txt",0,base,1)
   base = base + int(matDim_B / splits)
   os.system("perl $DSPLIB_ROOT/L2/tests/aie/common/scripts/matrix_mult_partition_shuffle.pl -f c" + str(split) + ".txt -r " + str(matDim_A) + " -c " + str(int(matDim_B / splits)) + " -T_DATA_A cfloat -T_DATA_B cfloat --splitRows")
   aiesimInp_gen(dataSamps,"tiled_c" + str(split) + ".txt","c" + str(split) + "_tmp.txt",iterCnt)
for split in range(8):
   merge_files("c" + str(split) + "_tmp.txt", "c" + str(split + 8) + "_tmp.txt", "c" + str(split) + "_final_output.txt", iterCnt)
os.chdir("../")

# Run: python3 plioGen.py 256 256 512 1 1
