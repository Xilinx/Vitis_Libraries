/*
 * Copyright 2019 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#include "xf_fintech_api.hpp"

using namespace std;
using namespace xf::fintech;

int main(int argc, char* argv[]) {
    // We expect 13 arguments: the program name, the solver & model parameters
    if (argc < 13) {
        std::cerr << "Usage: " << argv[0] << ": kappa eta sigma rho rd T K S V N m1 m2" << std::endl;
        return 1;
    }

    try {
        unsigned int argIndex = 1;
        double kappa = std::strtod(argv[argIndex++], NULL);
        double eta = std::strtod(argv[argIndex++], NULL);
        double sigma = std::strtod(argv[argIndex++], NULL);
        double rho = std::strtod(argv[argIndex++], NULL);
        double rd = std::strtod(argv[argIndex++], NULL);
        double T = std::strtod(argv[argIndex++], NULL);
        double K = std::strtod(argv[argIndex++], NULL);
        double S = std::strtod(argv[argIndex++], NULL);
        double V = std::strtod(argv[argIndex++], NULL);
        int N = std::atoi(argv[argIndex++]);
        int m1 = std::atoi(argv[argIndex++]);
        int m2 = std::atoi(argv[argIndex++]);

        std::cout << "Command Line Args:" << std::endl;
        std::cout << "kappa: " << kappa << std::endl;
        std::cout << "eta: " << eta << std::endl;
        std::cout << "sigma: " << sigma << std::endl;
        std::cout << "rho: " << rho << std::endl;
        std::cout << "rd: " << rd << std::endl;
        std::cout << "T: " << T << std::endl;
        std::cout << "K: " << K << std::endl;
        std::cout << "S: " << S << std::endl;
        std::cout << "V: " << V << std::endl;
        std::cout << "N: " << N << std::endl;
        std::cout << "m1: " << m1 << std::endl;
        std::cout << "m2: " << m2 << std::endl;
        std::cout << std::endl;

        // K - the strike price.
        // S - the stock price.
        // V - volatility of stock.
        // T - time to maturity.
        // kappa - mean reversion rate.
        // sig - the volatility of volatility.
        // rho - the correlation coefficient between price and variance.
        // eta - long run average price.
        // rd - risk-free domestic interest rate.
        // N - the number of timesteps
        // m1 - grid size for the S direction
        // m2 - grid size for the V direction

        // Model & Solver parameters passed in from command line

        int retval = XLNX_OK;

        std::vector<Device*> deviceList;
        Device* pChosenDevice;

        FDHeston fdHeston(m1, m2);

        double NPV;

        double delta;
        double vega;
        double gamma;
        double volga;
        double vanna;

        deviceList = DeviceManager::getDeviceList();

        if (deviceList.size() == 0) {
            printf("No matching devices found\n");
            exit(0);
        }

        // we'll just pick the first device in the list...
        pChosenDevice = deviceList[0];

        // initialise the device...
        retval = fdHeston.claimDevice(pChosenDevice);

        if (retval == XLNX_OK) {
            // run the heston model...

            // retval = fdHeston.run(S, K, rd, V, T, kappa, sigma, rho, eta, N, &NPV);
            retval =
                fdHeston.run(S, K, rd, V, T, kappa, sigma, rho, eta, N, &NPV, &delta, &vega, &gamma, &volga, &vanna);
        }

        if (retval == XLNX_OK) {
            std::cout << "Duration: " << fdHeston.getLastRunTime() << " us" << std::endl;
            std::cout << "NPV: " << NPV << std::endl;
            std::cout << "Delta: " << delta << std::endl;
            std::cout << "Vega: " << vega << std::endl;
            std::cout << "Gamma: " << gamma << std::endl;
            std::cout << "Volga: " << volga << std::endl;
            std::cout << "Vanna: " << vanna << std::endl;
        } else {
            std::cout << "ERROR: failed to calculate the NPV!!" << std::endl;
        }

        if (retval == XLNX_OK) {
            // release the device...
            retval = fdHeston.releaseDevice();
        }

    } catch (const std::exception& ex) {
        std::cout << ex.what() << std::endl;
    } catch (const std::string& ex) {
        std::cout << ex << std::endl;
    } catch (...) {
        std::cout << "Exception" << std::endl;
    }

    return 0;
}
