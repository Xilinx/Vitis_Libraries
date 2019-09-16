#!/usr/bin/env python3

# Ensure environmental variables i.e. paths are set to the named the modules
from xf_fintech_python import DeviceManager, CFBlackScholes, OptionType

# State test financial model
print("\nThe CFBlack Scholes financial model\n==================================================\n")

# Declaring Variables
deviceList = DeviceManager.getDeviceList("u250")
lastruntime = 0
# Example financial data to test the module as used in the C++ example script
numAssets = 100  # reduced from 100000 to 100 for clarity of script output - tested at 100000 samples
# Inputs
stockPriceList = [100.0] * numAssets
strikePriceList = [100.0] * numAssets
volatilityList = [0.1] * numAssets
riskFreeRateList= [0.025] * numAssets
timeToMaturityList = [1.0] * numAssets
# Outputs - declaring them as empty lists
optionPriceList = []
deltaList = []
gammaList = []
vegaList = []
thetaList = []
rhoList = []


# Identify which cards are installed and choose the first available U250 card, as defined in deviceList above
print("Found these {0} device(s):".format(len(deviceList)))
for x in deviceList:
    print(x.getName())
print("Choosing the first suitable card\n")
chosenDevice = deviceList[0]

# Selecting and loading into FPGA on chosen card the financial model to be used
CFBlackScholes = CFBlackScholes(numAssets)   # warning the lower levels to accomodate at least this figure
CFBlackScholes.claimDevice(chosenDevice)
#Feed in the data and request the result using tolerance method
print("\nRunning...")
result = CFBlackScholes.run(stockPriceList, strikePriceList, volatilityList, riskFreeRateList, timeToMaturityList, 
                            optionPriceList, deltaList, gammaList, vegaList, thetaList, rhoList, OptionType.Put, numAssets)
print("Done")
runtime = CFBlackScholes.lastruntime()

#Format output to match the example in C++, simply to aid comparison of results
print("+-------+-----------+----------------+--------------+---------------+---------------+---------------+")
print("| Index | Price     |     Delta      |     Gamma    |     Vega      |     Theta     |     Rho       |")
print("+-------+-----------+----------------+--------------+---------------+---------------+---------------+")
for loop in range(0, numAssets) :
    print(loop,"\t%9.5f"%optionPriceList[loop],"\t%9.5f"%deltaList[loop],"\t%9.5f"%gammaList[loop],"\t%9.5f"%vegaList[loop],"\t%9.5f"%thetaList[loop],"\t%9.5f"%rhoList[loop])



print("\nThis run took", str(runtime), "microseconds")

#Relinquish ownership of the card
CFBlackScholes.releaseDevice()
