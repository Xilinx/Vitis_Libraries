#!/usr/bin/env python3

# Ensure environmental variables i.e. paths are set to the named the modules
from xf_fintech_python import DeviceManager, CreditDefaultSwap
import sys

# Basic checking that the number of arguments are correct
if len(sys.argv) != 2:
    sys.exit("Incorrect number of arguments supplied - 1 expected - the name of the FPGA load - e.g. cds.xclbin")

# State test financial model
print("\nThe CreditDefaultSwap financial model\n==================================================\n")

# Declaring Variables
deviceList = DeviceManager.getDeviceList("u200")
lastruntime = 0
runtime = 0

# Example financial data to test the module - same as used in the C++ test script
# Inputs
ratesIRList     = [0.0300, 0.0335, 0.0366, 0.0394, 0.0418, 0.0439, 0.0458, 0.0475, 0.0490, 0.0503, 0.0514,
                   0.0524, 0.0533, 0.0541, 0.0548, 0.0554, 0.0559, 0.0564, 0.0568, 0.0572, 0.0575]
timesIRList     = [0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0, 
                   5.5, 6.0, 6.5, 7.0, 7.5, 8.0, 8.5, 9.0, 9.5, 10.0]
ratesHazardList = [0.005, 0.01, 0.01, 0.015, 0.010, 0.010]
timesHazardList = [0.0, 0.5, 1.0, 2.0, 5.0, 10.0]
notionalList    = [1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0]
recoveryList    = [0.15, 0.67, 0.22, 0.01, 0.80, 0.99, 0.001, 0.44]
maturityList    = [2.0, 3.0, 4.0, 5.55, 6.33, 7.27, 8.001, 9.999]
frequencyList   = [4, 12, 2, 1, 12, 4, 1, 12]
# Outputs - declaring them as empty lists
CDSSpreadList = []

# Identify which cards are installed and choose the first available u200 card, as defined in deviceList above
print("Found these {0} device(s):".format(len(deviceList)))
for x in deviceList:
    print(x.getName())
print("Choosing the first suitable card\n")
chosenDevice = deviceList[0]

# Selecting and loading into FPGA on chosen card the financial model to be used
##CFB76 = CFB76(numAssets)   # warning the lower levels to accomodate at least this figure
CreditDefaultSwap = CreditDefaultSwap(sys.argv[1])
CreditDefaultSwap.claimDevice(chosenDevice)
#Feed in the data and request the result using tolerance method
print("\nRunning...")
result = CreditDefaultSwap.run(timesIRList, ratesIRList, timesHazardList, ratesHazardList, notionalList, 
                               recoveryList, maturityList, frequencyList, CDSSpreadList)
print("Done")
runtime = CreditDefaultSwap.lastruntime()

# Display results
for i in range(len(CDSSpreadList)):
    print("CDSSpread [",i,"] = ",CDSSpreadList[i])
    i += 1

print("\nThis run took", str(runtime), "microseconds")

#Relinquish ownership of the card
CreditDefaultSwap.releaseDevice()
