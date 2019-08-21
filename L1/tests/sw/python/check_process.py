import time
import argparse
import os

def main(args): 
  allFound = False
  if args.ext:
    fileList = ['%s_%d.%s'%(args.basename, i, args.ext) for i in range(args.number) ]
  else:
    fileList = ['%s_%d'%(args.basename, i) for i in range(args.number) ]
  while True:
    fileFound = [os.path.exists(f) for f in fileList]
    func = lambda x, y : x and y
    allFound = reduce(func, fileFound)
    if allFound:
      break
    time.sleep(args.time)

if __name__== "__main__":
  parser = argparse.ArgumentParser(description='Generate random vectors and run test.')
  parser.add_argument('--basename', type=str, required=True, help='filename to check')
  parser.add_argument('--ext', type=str, help='file extension')
  parser.add_argument('--number', type=int, required=True, help='number of files')
  parser.add_argument('--time', type=int, default=600, help='number of seconds to poll')
  args = parser.parse_args()
  
  main(args)

