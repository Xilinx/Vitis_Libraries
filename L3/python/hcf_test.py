#!/usr/bin/env python3

# Ensure environmental variables i.e. paths are set to used the modules
from xf_fintech_python import DeviceManager, hcf_input_data,hcf 

# State test financial model
print("\nThe Heston Closed Form Model, with Multiple Options European Call")
print("=================================================================\n")

# Declaring Variables

# order is StockPrice, StrikePrice, RiskFreeInterestRate,Volatility,time to vest (years),
#          kappa the expected[Y-1] where Y is the random variable, lambda - mean jump per unit time,
#          lambda - mean jump per unit time, and finally the expected result

inputDataList =[]    # This is your list of input data , a list of lists
outputList = [] # This will be filled with a list of OptionPrices, one for each row of data input
numberOptions = 16
#Model Input Data
s0 = 80.0   # stock price at t=0
v0 = 0.1    # stock price variance at t=0
K = 100.0   # strike price
rho = -0.9  # correlation of the 2 Weiner processes
T = 1.0     # expiration time
r = 0.05    # risk free interest rate
vvol = 0.3  # volatility of volatility (sigma)
vbar = 0.04 # long term average variance (theta)
kappa = 1.5 # rate of reversion

# populate some data
for loop in range(0, numberOptions) :
    inputDataList.append(hcf_input_data())
    inputDataList[loop].s0 = s0 + (3 * loop)
    inputDataList[loop].v0= v0
    inputDataList[loop].K = K
    inputDataList[loop].rho = rho
    inputDataList[loop].T = T
    inputDataList[loop].r = r
    inputDataList[loop].kappa = kappa
    inputDataList[loop].vvol = vvol
    inputDataList[loop].vbar= vbar

print("[XF_FINTECH] ==========");
print("[XF_FINTECH] Parameters");
print("[XF_FINTECH] ==========");
print("[XF_FINTECH] Strike price                       = ", K);
print("[XF_FINTECH] Rho (Weiner process correlation)   = ", rho);
print("[XF_FINTECH] Time to maturity                   = ", T);
print("[XF_FINTECH] Risk free interest rate            = ", r);
print("[XF_FINTECH] Rate of reversion (kappa)          = ", kappa);
print("[XF_FINTECH] volatility of volatility (sigma)   = ", vvol);
print("[XF_FINTECH] Long term average variance (theta) = ", vbar);


deviceList = DeviceManager.getDeviceList("u250") # Look for U250 cards
lastruntime = 0  # Query this implementation

print("Found these {0} device(s):".format(len(deviceList)))
for x in deviceList:
    print(x.getName())

print("Choosing the first suitable card\n")
chosenDevice = deviceList[0]


hestonCF= hcf()
hestonCF.claimDevice(chosenDevice)

hestonCF.run(inputDataList, outputList, numberOptions) #This is the call to process contents of dataList

#Format output to match the example in C++, simply to aid comparison of results
print("[XF_FINTECH] Multiple [",numberOptions,"] Options European Call");
for loop in range(0, numberOptions) :
    print("[XF_FINTECH] Option %2d"%loop,"\tOptionPrice = %8.5f"%outputList[loop])

print("\nEnd of example.")


hestonCF.releaseDevice()
