#!/usr/bin/env python3

# Ensure environmental variables i.e. paths are set to the named the modules
from xf_fintech_python import DeviceManager, HullWhiteAnalytic
import random
import sys

# Basic checking that the number of arguments are correct
if len(sys.argv) != 2:
    sys.exit("Incorrect number of arguments supplied - 1 expected - the name of the FPGA load - e.g. hwa.xclbin")

# State test financial model
print("\nThe HullWhiteAnalytic financial model\n=====================================\n")

# Declaring Variables
deviceList = DeviceManager.getDeviceList("u200")
lastruntime = 0
runtime = 0

# note these values should match the generated kernel
N_k0 = 16;
LEN = 16;
# Inputs - note that due to the random nature of the input data the output will also vary
a = 0.10;
sigma = 0.01;
ratesList = [0.0020, 0.0050, 0.0070, 0.0110, 0.0150, 0.0180, 0.0200, 0.0220, 0.0250, 0.0288, 0.0310, 0.0340, 0.0, 0.0 ,0.0, 0.0]
timesList = [0.25, 0.50, 0.75, 1.00, 1.50, 2.00, 3.00, 4.00, 5.00, 10.0, 20.0, 30.0, 0.0, 0.0, 0.0, 0.0]
tList = [random.uniform(0,15) for iteration in range(LEN)]
#print("tist is",tList)
TList = [(random.uniform(0,15)+1+tList[iteration]) for iteration in range(LEN)]
#print("TList is",TList)
# Output - declaring it as an empty list
PList= []

# Identify which cards are installed and choose the first available u200 card, as defined in deviceList above
print("Found these {0} device(s):".format(len(deviceList)))
for x in deviceList:
    print(x.getName())
print("Choosing the first suitable card\n")
chosenDevice = deviceList[0]

# Selecting and loading into FPGA on chosen card the financial model to be used
HullWhiteAnalytic = HullWhiteAnalytic(sys.argv[1])
HullWhiteAnalytic.claimDevice(chosenDevice)
#Feed in the data and request the result using tolerance method
print("\nRunning...")
result = HullWhiteAnalytic.run(a, sigma, timesList, ratesList, tList, TList, PList)
print("Done")
runtime = HullWhiteAnalytic.lastruntime()

# Display results
for i in range(len(PList)):
    print("HullWhite Spread [",i,"] = ",PList[i])
    i += 1

print("\nThis run took", str(runtime), "microseconds")

#Relinquish ownership of the card
HullWhiteAnalytic.releaseDevice()
