import time
import functools
import argparse
import os, sys

def Format(x):
  f_dic = {0:'th', 1:'st', 2:'nd',3:'rd'}
  k = x % 10
  if k>=4:
    k=0
  return "%d%s"%(x, f_dic[k])


def poll(fileList, t, progress = 80):
  id = 0
  while True:
    fileFound = [os.path.exists(f) for f in fileList]
    func = lambda x, y : x and y
    allFound = functools.reduce(func, fileFound)
    if allFound:
      print("Polling finished.")
      break
    id+=1
    print("Sleeping for %ds."%(t))
    perT = t / progress
    sys.stdout.write('%s: [='%Format(id))
    for i in range(progress):
      sys.stdout.write('\b=%d'%(i%10))
      sys.stdout.flush()
      time.sleep(perT)
    sys.stdout.write('\b]\n')


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

