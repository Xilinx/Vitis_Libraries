import json

def loadProfile(filePath):
  with open(filePath, 'r') as fh:
    profile = json.loads(fh.read())
    return profile


def writeProfile(profile, filePath):
  with open(filePath, 'w') as fh:
    fh.write(json.dumps(profile, indent=2))
 

def main():

  profile=dict()

  profile['tclPath'] = r"./amaxmin/build/run.tcl"
  profile['dataPath'] = r"./amaxmin/data/"

  profile['b_csim'] = True
  profile['b_synth'] = True
  profile['b_cosim'] = True

  profile['dataType'] = 'double'
  profile['dataWidth']= 64

  profile['op'] = 'amax'

  profile['parEntries'] = 4
  profile['vectorSize'] = 8192

  writeProfile(profile, r'profile.json')


if __name__=='__main__':
  main()
