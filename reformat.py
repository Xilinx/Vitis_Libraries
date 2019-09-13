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
import subprocess, shlex
import pdb
import argparse
import os

def search_files(path, r, types):
  pathList = [path]
  fileList = list()
  while pathList:
      p = pathList.pop(0)
      for file in os.listdir(p):
          fullpath =os.path.join(p, file)
          if r and os.path.isdir(fullpath):
              pathList.append(fullpath)
          elif os.path.isfile(fullpath):
              _, ext = os.path.splitext(fullpath)
              if ext in types:
                  fileList.append(fullpath)
  return fileList

def clangFormat(files):
    try:
      for f in files:
        command ="clang-format -style=file -i %s"%f
        args = shlex.split(command)
        subprocess.call(args)
    except OSError as err:
        if e.errno == os.errno.ENOENT:
            print("clang-format is not found.")
def root_path():
    return os.path.abspath(os.sep)
def checkFormatFile(path):
    root = root_path()
    while True:
        formatFile=os.path.abspath(os.path.join(path, r'.clang-format'))
        if os.path.exists(formatFile):
            print("Format file %s will be used."%formatFile)
            break
        elif path == root:
            print("No format file is found, default will be used.")
            break
        else:
            path = os.path.realpath(os.path.join(path, os.pardir))

def main(path, r, exts):
    checkFormatFile(path)
    files = search_files(path, r, exts)
    print(files)
    clangFormat(files)

if __name__=='__main__':
    parser = argparse.ArgumentParser(description='clang-format in place')
    parser.add_argument('-r', help='recursive', action="store_true")
    parser.add_argument('path', type=str, metavar='Path', help='path to execute')
    parser.add_argument('ext', nargs='*', default=['.hpp', '.cpp', '.h'], metavar='fileExtensions',
          help='extensions of files to be processed')

    args=parser.parse_args()
    main(args.path, args.r, args.ext)

