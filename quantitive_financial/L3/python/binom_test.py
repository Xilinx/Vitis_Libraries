#!/usr/bin/env python3

# Ensure environmental variables i.e. paths are set to used the modules
from xf_fintech_python import DeviceManager, BinomialTreeInputDataTypeDouble, BinomialTree, OptionType
import array

# State test financial model
print("\nThe Binomial financial model\n========================================\n")


# The Binomial bitstream is currently built for a U200
deviceList = DeviceManager.getDeviceList("u200")

print("Found these {0} device(s):".format(len(deviceList)))
for x in deviceList:
    print(x.getName())

print("Choosing the first suitable card\n")
chosenDevice = deviceList[0]


dataList = []

for i in range(10):
    dataList.append(BinomialTreeInputDataTypeDouble())

dataList[0].S = 110.0
dataList[0].K = 100.0
dataList[0].T = 1.0
dataList[0].rf = 0.05
dataList[0].V = 0.2
dataList[0].q = 0
dataList[0].N = 1024

dataList[1].S = 111.1
dataList[1].K = 100.0
dataList[1].T = 1.0
dataList[1].rf = 0.05
dataList[1].V = 0.2
dataList[1].q = 0
dataList[1].N = 1024



bto = BinomialTree()

bto.claimDevice(chosenDevice)

outputlist = []

bto.run(dataList, outputlist, BinomialTree.OptionTypeEuropeanPut)


print("Output Result: num ", len(outputlist))
for i in range(len(outputlist)):
    print(outputlist[i])


bto.releaseDevice()

