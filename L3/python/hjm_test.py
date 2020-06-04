#!/usr/bin/env python3

# Ensure environmental variable i.e. paths are set to used the modules
from xf_fintech_python import DeviceManager, HJM
import numpy as np
import argparse

def zcbAnalytical(rawData, maturity, tau = 0.5):
    # Take last row 
    fc = np.copy(rawData[rawData.shape[0] - 1])
    fc *= 0.01
    accum = 0.0
    for i in range(int(maturity / tau)):
        accum += fc[i]
    return np.exp(-tau * accum)


def zcbExample(hjm, hist_data, maturity, paths):
    seeds = (np.random.rand(N_FACTORS * MC_UN) * 1000).astype(int)
    print("Using seeds " + str(seeds))

    outPrice = []

    hjm.run(list(hist_data.flat), list(seeds.flat), outPrice, tenors, curves, paths, maturity, maturity)
    runtime = hjm.lastruntime()
    analyticalPrice = zcbAnalytical(hist_data, maturity)
    print("[CPU]  ZCB calculated analytically:       %10.6f" % analyticalPrice)
    print("[FPGA] ZCB calculated with HJM framework: %10.6f" % outPrice[0])
    print("[FPGA] Runtime = %d" %runtime, "us")
    diff = (outPrice[0] - analyticalPrice) / analyticalPrice * 100
    print("    Diff = %.4f" % diff, "%")


parser = argparse.ArgumentParser(description='Example of Heath-Jarrow-Morton framework running on a FPGA')
parser.add_argument('data_in', type=str, help='Path to csv with historical rates data')
parser.add_argument('load', type=str, help='filename of xlcbin load, e.g. hjm.xclbin')

args = parser.parse_args()

N_FACTORS = 3
MC_UN = 4

hist_data = np.loadtxt(args.data_in, delimiter=',')
tenors = hist_data.shape[1]
curves = hist_data.shape[0]

xclbin_load = (args.load)

print("\nThe Heath-Jarrow-Morton model\n=================================\n")

# Program variables
deviceList = DeviceManager.getDeviceList("u200")
lastruntime = 0
runtime = 0

# Identify which cards installed and choose the first available U200 card
print("Found these {0} device(s):".format(len(deviceList)))
for x in deviceList:
    print(x.getName())
chosenDevice = deviceList[0]
print("Choosing the first, ", str(chosenDevice), "\n")


# Selecting and loadings into FPGA of chosen card the financial model to be used
hjm = HJM(xclbin_load)
hjm.claimDevice(chosenDevice)

# Examples of possible operations, showing MC convergence of prices
print("Example 1) Pricing ZCB of maturity 10Y with 50 MonteCarlo paths")
zcbExample(hjm, hist_data, 10.0, 50)
print("Example 2) Pricing ZCB of maturity 10Y with 100 MonteCarlo paths")
zcbExample(hjm, hist_data, 10.0, 100)
print("Example 3) Pricing ZCB of maturity 10Y with 200 MonteCarlo paths")
zcbExample(hjm, hist_data, 10.0, 200)
print("Example 4) Pricing ZCB of maturity 10Y with 400 MonteCarlo paths")
zcbExample(hjm, hist_data, 10.0, 400)

hjm.releaseDevice()
print("End of example/test.\n")
