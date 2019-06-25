import json


class Profile:
  def __init__(self):
    self.m_profile = dict()

  def loadProfile(self, filePath):
    with open(filePath, 'r') as fh:
      self.m_profile = json.loads(fh.read())
           
  def writeProfile(self, filePath):
    with open(filePath, 'w') as fh:
      fh.write(json.dumps(self.m_profile, indent=2))
                   
def main():

  profile= Profile()
   
  profile.m_profile['b_csim'] = True
  profile.m_profile['b_synth'] = True
  profile.m_profile['b_cosim'] = True

  profile.m_profile['dataTypes'] = [('float',64), ('float',32), ('int',16), ('int',8)]

  profile.m_profile['op'] = 'amax'

  profile.m_profile['parEntries'] = 4
  profile.m_profile['vectorSizes'] = [128, 8192]
  profile.m_profile['valueRange'] = [-1024, 1024]
  profile.m_profile['numSimulation'] = 2

  profile.writeProfile(r'profile.json')


if __name__=='__main__':
  main()

