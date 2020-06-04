#!/usr/bin/env python3

#Ensure environmental variable i.e. paths are set to used the modules
from xf_fintech_python import DeviceManager, LMM
import numpy as np
from scipy.stats import norm
import sys

# Basic checking that the number of arguments are correct
if len(sys.argv) != 2:
    sys.exit("Incorrect number of arguments supplied - 1 expected - the name of the FPGA load - e.g. lmmratchet.xclbin")

def genSeeds():
    return list((np.random.rand(UN) * 1000).astype(int))

cVolas = [0.2366, 0.2487, 0.2573, 0.2564, 0.2476, 0.2376, 0.2252, 0.2246, 0.2223]
lRates = [0.0112, 0.0118, 0.0123, 0.0127, 0.0132, 0.0137, 0.0145, 0.0154, 0.0163, 0.0174]
noTenors = 10
noPaths = 10000
notional = 1e6
UN = 4


def capAnalytical(caprate, tau = 0.5):
    clet_prices = np.array([])
    for i in range(1, noTenors - 1):
        vol = cVolas[i - 1]
        L = lRates[i]
        T = tau * i
        d1 = (np.log(L / caprate) + 0.5 * vol * vol * T) / (vol * np.sqrt(T))
        d2 = d1 - (vol * np.sqrt(T))

        cr = (1/tau) * np.log(1 + np.sum(lRates[0:i+1]) / (i + 1) * tau)
        base_price = notional * tau * (L * norm.cdf(d1) - caprate * norm.cdf(d2))
        caplet_price = np.exp(-cr * tau * (i + 1)) * base_price
        np.append(clet_prices, caplet_price)
    return np.sum(clet_prices)


def capTest(caprate, paths):
    outPrice = []
    lmm.runCap(lRates, cVolas, genSeeds(), outPrice, noTenors, paths, 0.2, 1e6, caprate)
    expected = capAnalytical(caprate)
    print("\t[CPU]  Analytical Cap price: %.3f" % expected)
    print("\t[FPGA] LMM Cap price: %.3f" % outPrice[0])
    print("\t[FPGA] Runtime = %d" % lmm.lastruntime(), "us")
    diff = (outPrice[0] - expected) / expected * 100
    print("\t\tDiff = %.4f" % diff, "%")

def ratchetCapTest(spread, kappa0, paths):
    outPrice = []
    lmm.runRatchetCap(lRates, cVolas, genSeeds(), outPrice, noTenors, paths, 0.2, 1e6, spread, kappa0)
    print("\t[FPGA] LMM Ratchet Cap price: %.3f" % outPrice[0])
    print("\t[FPGA] Runtime = %d" % lmm.lastruntime(), "us")

def ratchetFloaterTest(rfX, rfY, rfAlpha, paths):
    outPrice = []
    lmm.runRatchetFloater(lRates, cVolas, genSeeds(), outPrice, noTenors, paths, 0.2, 1e6, rfX, rfY, rfAlpha)
    print("\t[FPGA] LMM Ratchet Floater price: %.3f" % outPrice[0])
    print("\t[FPGA] Runtime = %d" % lmm.lastruntime(), "us")

print("\nThe LIBOR Market Model\n======================================\n")

# Program variables
deviceList = DeviceManager.getDeviceList("u200")

# Identify which cards installed and choose the first available U200 card
print("Found there {0} device(s):".format(len(deviceList)))
for x in deviceList:
    print(x.getName())
chosenDevice = deviceList[0]
print("Choosing the first, ", str(chosenDevice), "\n")

# Selecting and loading into FPGA of chosen card the LMM model to be used
lmm = LMM(sys.argv[1])

# Examples of possible operations for Cap pricing

print("\n-------------------------------")
print("        LMM Cap Pricing        ")
print("-------------------------------\n")

lmm.claimDeviceCap(chosenDevice)
print("CAP Example 1) K = 1.1%, 1K paths")
capTest(0.011, 1000)
print("CAP Example 2) K = 1.1%, 10K paths")
capTest(0.011, 10000)
print("CAP Example 3) K = 0.5%, 10K paths")
capTest(0.005, 10000)
print("CAP Example 4) K = 2.0%, 10K paths")
capTest(0.02, 10000)
lmm.releaseDevice()

# Examples of possible operations for Ratchet Floater pricing

print("\n-------------------------------------------")
print("        LMM Ratchet Floater Pricing        ")
print("-------------------------------------------\n")

lmm.claimDeviceRatchetFloater(chosenDevice)
print("RATCHET FLOATER Example 1) X = 0.15%, Y = 0.15%, alpha = 0.01%, 10K paths")
ratchetFloaterTest(0.0015, 0.0015, 0.0001, 10000)
print("RATCHET FLOATER Example 2) X = 0.15%, Y = 0.15%, alpha = 0.1%, 10K paths")
ratchetFloaterTest(0.0015, 0.0015, 0.001, 10000)
print("RATCHET FLOATER Example 3) X = 0.25%, Y = 0.15%, alpha = 0.05%, 10K paths")
ratchetFloaterTest(0.0025, 0.0015, 0.0005, 10000)
print("RATCHET FLOATER Example 4) X = 0.20%, Y = 0.10%, alpha = 0.05%, 10K paths")
ratchetFloaterTest(0.002, 0.001, 0.0005, 10000)
lmm.releaseDevice()

# Examples of possible operations for Ratchet Cap pricing

print("\n---------------------------------------")
print("        LMM Ratchet Cap Pricing        ")
print("---------------------------------------\n")

lmm.claimDeviceRatchetCap(chosenDevice)
print("RATCHET CAP Example 1) spread = 0.5%, kappa0 = 0.5%, 10K paths")
ratchetCapTest(0.005, 0.005, 10000)
print("RATCHET CAP Example 1) spread = 0.1%, kappa0 = 0.1%, 10K paths")
ratchetCapTest(0.001, 0.001, 10000)
print("RATCHET CAP Example 1) spread = 0.5%, kappa0 = 0.1%, 10K paths")
ratchetCapTest(0.005, 0.001, 10000)
print("RATCHET CAP Example 1) spread = 0.1%, kappa0 = 0.5%, 10K paths")
ratchetCapTest(0.001, 0.005, 10000)
lmm.releaseDevice()


print("End of example/test.\n")
