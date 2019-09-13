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
import os
import subprocess
import pdb
import argparse
def findFiles(extension, path = os.path.curdir, r=False):
  pathList = [path]
  fileList = list()
#  pdb.set_trace()
  while pathList:
    curPath = pathList.pop(0)
    dirs = os.listdir(curPath)
    for d in dirs:
      fullPath = os.path.abspath(os.path.join(curPath, d))
      if r and os.path.isdir(fullPath):
        pathList.append(fullPath)
      elif os.path.isfile(fullPath):
        filename, ext =  os.path.splitext(fullPath)
        if ext in extension:
          fileList.append(fullPath)
  return fileList
        
def gitReName(fileList, newExt):
  namePairs = list()
  for file in fileList:
    basename = os.path.basename(file)
    filename, ext = os.path.splitext(basename)
    newBaseName = filename + newExt
    path, ext = os.path.splitext(file)
    newFile = path+newExt
    command = ['git', 'mv', file, newFile]
    subprocess.call(command)
#    command = ['git', 'commit', file, newFile, '-m','rename %s to %s'%(basename, newBaseName)]
#    subprocess.call(command)
    namePairs.append((basename, newBaseName))
  return namePairs

def replaceString(files, pairs):
#  pdb.set_trace()
  for f in files:
    modified = False
    with open(f, 'r') as fr:
      string = fr.read()
    for pair in pairs:
      str0 = pair[0]
      str1 = pair[1]
      if string.find(str1) < 0:
        string = string.replace(str0, str1)
        modified = True
    if modified:
      with open(f,'w') as fw:
        fw.write(string)

def main(path, r):
  hFiles = findFiles(['.h'], path, r)
  filePair = gitReName(hFiles, '.hpp')
  rFiles = findFiles(['.hpp', '.cpp'], path, r)
  replaceString(rFiles, filePair)
  #print(filePair)

if __name__=='__main__':
  parser = argparse.ArgumentParser(description='rename header file extension')
  parser.add_argument('-r', help='recursive', action="store_true")
  parser.add_argument('path', type=str, metavar='Path', help='path to execute')

  args=parser.parse_args()
  main(args.path, args.r)

