#!/usr/bin/env python3

# Ensure environmental variables i.e. paths are set to used the modules
from xf_fintech_python import DeviceManager, MCAmerican, OptionType

print("\nThe MCAmerican financial model\n==============================\n")

# Program Variables
deviceList = DeviceManager.getDeviceList("u250")
lastruntime = 0
runtime = 0


# Identify which cards installed and choose the first available U250 card
print("Found these {0} device(s):".format(len(deviceList)))
for x in deviceList:
    print(x.getName())
chosenDevice = deviceList[0]
print("Choosing the first, ",str(chosenDevice),"\n")


# Selecting and loading into FPGA on chosen card the financial model to be used
mcAmerican = MCAmerican()
mcAmerican.claimDevice(chosenDevice)

# Examples of possible operations
print("\nA two examples follow below\n===========================\n")

print("Example 1) Running with tolerance and a single asset value")
print("----------------------------------------------------------")
result = mcAmerican.run(OptionType.Put, 36.0, 40.0, 0.06, 0.0, 0.20, 1.0, 0.02)
#Direcly passing in OptionType.Put, StockPrice, StrikePrice, RiskFreeRate, DividendYield,
#                   Volatility, TimeToMaturity, RequiredTolerance
print("Option Price = {0}".format(result[1]))
runtime = mcAmerican.lastruntime()
print("This run took",str(runtime), "microseconds\n")

# Next example mirroring the C++ for comparision.
# This is a test to prove the results are not buffered, by repeatedly calling the 
# device 100 times each time varying the input data

print("\nExample 2) Device called 100 times, varying the input data each time")
print("--------------------------------------------------------------------")

iterations = 100
#Financial variables
initialStockPrice = 36.0
initialStrikePrice = 40.0
initialRiskFreeRate = 0.06
initialDividendYield = 0.0
initialVolatility = 0.2
initialTimeToMaturity = 1.0 # in years
initialRequiredTolerance = 0.02
varianceFactor = 0.01 # this is used to vary the data in each iteration of the loop

print("+-----------+-------------+--------------+-----------+------------+------------+--------------+----------------+")
print("| Iteration | Stock Price | Strike Price | Risk Free | Div. Yield | Volatility | Option Price | Execution Time |")
print("+-----------+-------------+--------------+-----------+------------+------------+--------------+----------------+")

for loop in range(0,iterations) :
    variance = (1.0 + (varianceFactor * loop))

    stockPrice = initialStockPrice * variance
    strikePrice = initialStrikePrice * variance
    riskFreeRate = initialRiskFreeRate * variance
    dividendYield = initialDividendYield * variance
    volatility = initialVolatility * variance

    timeToMaturity = initialTimeToMaturity
    requiredTolerance = initialRequiredTolerance

    runtime = 0

    result = mcAmerican.run(OptionType.Put, stockPrice, strikePrice, riskFreeRate, dividendYield,
                            volatility, timeToMaturity, requiredTolerance)

    runtime = mcAmerican.lastruntime()

    print("|%10d"%loop,"|%12.4f"%stockPrice,"|%13.4f"%strikePrice,"|%10.4f"%riskFreeRate,
          "|%11.4f"%dividendYield, "|%11.4f"%volatility,
          "|%13.4f"%result[1],"|%12d uS |"%runtime) 

print("+-----------+-------------+--------------+-----------+------------+------------+--------------+----------------+")

mcAmerican.releaseDevice()
print("\nEnd of example/test.\n")
