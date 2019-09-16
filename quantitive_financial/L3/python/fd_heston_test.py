#!/use/bin/env python3

# Ensure environmental variables i.e. paths are set to used the modules
from xf_fintech_python import DeviceManager, FDHeston
import sys

# State test financial model
print("\nThe Heston FD financial model\n========================================\n")

# Declaring Variables
deviceList = DeviceManager.getDeviceList("u250")   # This should could be either u200 or u250, possibly others t.b.c.
sGridOutput = []
vGridOutput = []
priceGridOutput = []

lastruntime = 0


# In this example financial data to test the module is fed in via the command line. Below is an example
# ./heston_fd_test.py 1.5 0.04 0.3 -0.9 0.025 1 100 200 1 200 128 64
# and the expected value output for NPV is 111.832977

# Basic checking that the number of arguments are correct
if len(sys.argv) != 13:
    sys.exit("Incorrect number of arguments supplied - 12 expected")

#Take each suffixed value and give it a meaningful name and convert from arv string format to numerical
meanReversionRate_kappa = float(sys.argv[1])
longRunAveragePrice_eta = float(sys.argv[2])
volatilityOfVolatility_sigma = float(sys.argv[3])
correlationCoefficient_rho = float(sys.argv[4])
riskFreeDomesticInterestRate_rd = float(sys.argv[5])
expirationtime_T = float(sys.argv[6])
strikePrice_K = float(sys.argv[7])
stockPrice_S = float(sys.argv[8])
volatility_V = float(sys.argv[9])
numberOfTimesteps_N = int(sys.argv[10])
gridSizeForTheSdirection_m1 = int(sys.argv[11])
gridSizeForTheSdirection_m2 = int(sys.argv[12])
#Repeat back those values, the last three are integers.
print("So meanReversionRate_kappa is ",type(meanReversionRate_kappa)," and value is ",meanReversionRate_kappa)
print("and longRunAveragePrice_eta is ",type(longRunAveragePrice_eta)," and value is ",longRunAveragePrice_eta)
print("and volatilityOfVolatility_sigma is ",type(volatilityOfVolatility_sigma)," and value is ",volatilityOfVolatility_sigma)
print("and correlationCoefficient_rho is ",type(correlationCoefficient_rho)," and value is ",correlationCoefficient_rho)
print("and riskFreeDomesticInterestRate_rd is ",type(riskFreeDomesticInterestRate_rd)," and value is ",riskFreeDomesticInterestRate_rd)
print("and expirationtime_T is",type(expirationtime_T)," and value is ",expirationtime_T)
print("and strikePrice_K is ",type(strikePrice_K)," and value is ",strikePrice_K)
print("and stockPrice_S is ",type(stockPrice_S)," and value is ",stockPrice_S)
print("and volatility_V is ",type(volatility_V)," and value is ",volatility_V)
print("and numberOfTimesteps_N is ",type(numberOfTimesteps_N)," and value is ",numberOfTimesteps_N)
print("and gridSizeForTheSdirection_m1 is ",type(gridSizeForTheSdirection_m1)," and value is ",gridSizeForTheSdirection_m1)
print("and gridSizeForTheSdirection_m2 is ",type(gridSizeForTheSdirection_m2)," and value is ",gridSizeForTheSdirection_m2)
# Variables set and arrays for results, where necessary set


# Identify which cards are installed and choose the first available U250 card
print("Found these {0} device(s):".format(len(deviceList)))
for x in deviceList:
    print(x.getName())
print("Choosing the first suitable card\n")
chosenDevice = deviceList[0]

# Selecting and loading into FPGA on chosen card the financial model to be used
hestonFD = FDHeston()
hestonFD.claimDevice(chosenDevice)


#execute the calculation
print("\nRunning with number of steps specified,duplicating the c++ demonstration")
result = hestonFD.run(stockPrice_S, strikePrice_K, riskFreeDomesticInterestRate_rd, volatility_V, expirationtime_T, meanReversionRate_kappa, volatilityOfVolatility_sigma, correlationCoefficient_rho, longRunAveragePrice_eta, numberOfTimesteps_N )
print("\nSo the single output NPV is returning ",str(result))

runtime = hestonFD.lastruntime()
print("This run took", str(runtime), "microseconds") # display how long the processing took


print("\nNow running without specifying a number of steps")
result = hestonFD.run(stockPrice_S, strikePrice_K, riskFreeDomesticInterestRate_rd, volatility_V, expirationtime_T, meanReversionRate_kappa, volatilityOfVolatility_sigma, correlationCoefficient_rho, longRunAveragePrice_eta)
print("\nSo the single output NPV is returning ",str(result))

runtime = hestonFD.lastruntime()
print("This run took", str(runtime), "microseconds")


print("\nNow running with number of steps and feeding in dimensions of sgrid and vgrid, expecting arrays of results")
result = hestonFD.run(stockPrice_S, strikePrice_K, riskFreeDomesticInterestRate_rd, volatility_V, expirationtime_T, meanReversionRate_kappa, volatilityOfVolatility_sigma, correlationCoefficient_rho, longRunAveragePrice_eta, numberOfTimesteps_N, gridSizeForTheSdirection_m1, gridSizeForTheSdirection_m2, sGridOutput, vGridOutput, priceGridOutput)
print("\nSo the arrayed output is returning \n") # As there are a lot have gone with csv format output in the example
print("\nsGridvalues are: ")
for i in range(len(sGridOutput)):
                print(sGridOutput[i], end = ',')
                i += 1
print("\nvGridvalues are: ")
for i in range(len(vGridOutput)):
                print(vGridOutput[i], end = ',')
                i += 1
print("\npriceGridvalues are: ")
for i in range(len(priceGridOutput)):
                print(priceGridOutput[i], end = ',')
                i += 1

runtime = hestonFD.lastruntime()
print("\nThis run took", str(runtime), "microseconds")


#Relinquish ownership of the card
hestonFD.releaseDevice()
