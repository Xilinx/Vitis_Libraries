#!/usr/bin/env python3

# Ensure environmental variables i.e. paths are set to used the modules
from xf_fintech_python import DeviceManager, PopMCMC
#import array

# State test financial model
print("\nThe MCMC financial model\n==============================\n")

# Variables
deviceList = DeviceManager.getDeviceList("u250")
outputData = []
lastruntime = 0


# Identify which cards installed and choose the first available U250 card
print("Found these {0} device(s):".format(len(deviceList)))
for x in deviceList:
    print(x.getName())
chosenDevice = deviceList[0]
print("Choosing the first, ",str(chosenDevice),"\n")


# Selecting and loading into FPGA on chosen card the financial model to be used
PopMCMC = PopMCMC()
PopMCMC.claimDevice(chosenDevice)

# Example
print("\nAn  example follows below,\n\nint samples, int burninSamples, double sigma, list output");
result = PopMCMC.run( 5000, 500, 0.4, outputData)
print("Output Result: num", len(outputData))
for i in range(len(outputData)):
                print(outputData[i])
                #i += 1

# Report runtime
runtime = PopMCMC.lastruntime()
print("This run took",str(runtime), "microseconds\n")


PopMCMC.releaseDevice()
