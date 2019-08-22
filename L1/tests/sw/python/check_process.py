import time
import functools
import argparse
import os, sys

def poll(fileList, t, progress = 30):
  while True:
    fileFound = [os.path.exists(f) for f in fileList]
    func = lambda x, y : x and y
    allFound = functools.reduce(func, fileFound)
    if allFound:
      print("Poll finished.")
      break
    print("Poll sleep for %ds."%t)
    perT = t / progress
    print('[', end="")
    for i in range(progress):
      print('=', end="")
      sys.stdout.flush()
      time.sleep(perT)
    print(']')


def merge(fileList, filename):
  with open(filename, 'w+') as f:
    for file in fileList:
      with open(file, 'r+') as fr:
        f.write(fr.read())

def main(args): 
  fileList = ['%s_%d.%s'%(args.basename, i, args.ext) for i in range(args.number) ]
  poll(fileList, args.time)
  merge(fileList, '%s.%s'%(args.basename,args.ext))

if __name__== "__main__":
  parser = argparse.ArgumentParser(description='Generate random vectors and run test.')
  parser.add_argument('--basename', type=str, default='statistics', help='filename to check')
  parser.add_argument('--number', type=int, required=True, help='number of files')
  parser.add_argument('--ext', type=str, default='rpt', help='file extension')
  parser.add_argument('--time', type=int, default=60, help='number of seconds to poll')
  args = parser.parse_args()
  
  main(args)

