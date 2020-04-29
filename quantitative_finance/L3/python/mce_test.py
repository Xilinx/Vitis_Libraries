#!/usr/bin/env python3

# Ensure environmental variables i.e. paths are set to used the modules
from xf_fintech_python import DeviceManager, MCEuropean, OptionType
import array

# State test financial model
print("\nThe MCEuropean financial model\n==============================\n")

# Variables
deviceList = DeviceManager.getDeviceList("u250")
lastruntime = 0

# Identify which cards installed and choose the first available U250 card
print("Found these {0} device(s):".format(len(deviceList)))
for x in deviceList:
    print(x.getName())
chosenDevice = deviceList[0]
print("Choosing the first, ",str(chosenDevice),"\n")


# Selecting and loading into FPGA on chosen card the financial model to be used
mcEuropean = MCEuropean()
mcEuropean.claimDevice(chosenDevice)

# Examples of possible operations
print("\nA few examples follow below\n\nRunning with tolerance...single asset value");
result = mcEuropean.run(OptionType.Put, 36.0, 40.0, 0.06, 0.0, 0.20, 1.0, 0.02)
print("Option Price = {0}".format(result[1]))
runtime = mcEuropean.lastruntime()
print("This run took",str(runtime), "microseconds\n")

#print("Running with tolerance...single asset value");
#result = mcEuropean.run(OptionType.Put, 36.0, 40.0, 0.06, 0.0, 0.20, 1.0, 0.02)
#print("Output Results: num", len(outputResults))
# IH - to test - despite this only returning one reather than many values should still work
#for i in range(len(outputResults)):
#                print(outputResults[i])
#                i += 1


print("Running for required num samples...single asset value");
result = mcEuropean.run(OptionType.Put, 36.0, 40.0, 0.06, 0.0, 0.20, 1.0, 16383)
print("Option Price = {0}".format(result[1]))
runtime = mcEuropean.lastruntime()
print("This run took",str(runtime), "microseconds\n")

##print("Running for required num samples...single asset value");
##result = mcEuropean.run(OptionType.Put, 36.0, 40.0, 0.06, 0.0, 0.20, 1.0, 16383)
##print("Output Results: num", len(outputResults))
##for i in range(len(outputResults)):
##                print(outputResults[i])
##                i += 1


##initialise some arrays of asset data that we will use for out next set of calls...
optionTypeList     	= [OptionType.Put, OptionType.Put, OptionType.Put	]
stockPriceList     	= [36.0, 		   38.0, 		   40.0				]
strikePriceList    	= [40.0, 		   42.0, 		   44.0				]
riskFreeRateList   	= [0.06, 		   0.06, 		   0.06				]
dividendYieldList  	= [0.0, 		   0.0, 		   0.0				]
volatilityList     	= [0.20, 		   0.20, 		   0.20				]
timeToMaturityList 	= [1.0, 		   1.0, 		   1.0				]
toleranceList      	= [0.02, 		   0.02, 		   0.02				]
	                                                   
requiredSamplesList	= [16383,		   16383,          16383			]


print("Running with tolerance...with arrays of asset values");
result = mcEuropean.run(optionTypeList, stockPriceList, strikePriceList, riskFreeRateList, dividendYieldList, volatilityList, timeToMaturityList, toleranceList)
print("Option Prices = {0}".format(result[1]))
runtime = mcEuropean.lastruntime()
print("This run took",str(runtime), "microseconds\n")

#outputResults = []
#print("Running with tolerance...with arrays of asset values");
#result = mcEuropean.run(optionTypeList, stockPriceList, strikePriceList, riskFreeRateList, dividendYieldList, volatilityList, timeToMaturityList, toleranceList, outputResults)
#print("Output Results: num", len(outputResults))
#for i in range(len(outputResults)):
#                print(outputResults[i])
#                i += 1
#runtime = mcEuropean.lastruntime()


#print("Running with num samples...with arrays of asset values")
#result = mcEuropean.run(optionTypeList, stockPriceList, strikePriceList, riskFreeRateList, dividendYieldList, volatilityList, timeToMaturityList, requiredSamplesList)
#print("Option Prices = {0}".format(result[1]))

outputResults = []
print("Running with num samples...with arrays of asset values")
result = mcEuropean.run(optionTypeList, stockPriceList, strikePriceList, riskFreeRateList, dividendYieldList, volatilityList, timeToMaturityList, requiredSamplesList, outputResults)
print("Output Results: num", len(outputResults))
for i in range(len(outputResults)):
                print(outputResults[i])
                i += 1
runtime = mcEuropean.lastruntime()
print("This run took", str(runtime), "microseconds")

mcEuropean.releaseDevice()
