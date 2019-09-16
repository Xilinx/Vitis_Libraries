#ifndef _XF_FINTECH_KERNEL_MCASIANASENGINE_HPP_
#define _XF_FINTECH_KERNEL_MCASIANASENGINE_HPP_

typedef double TEST_DT;

extern "C" void kernel_MCAsianAS_0(TEST_DT underlying,
                                   TEST_DT volatility,
                                   TEST_DT dividendYield,
                                   TEST_DT riskFreeRate, // model parameter
                                   TEST_DT timeLength,
                                   TEST_DT strike,
                                   int optionType, // option parameter
                                   TEST_DT outputs[1],
                                   TEST_DT requiredTolerance,
                                   unsigned int requiredSamples,
                                   unsigned int timeSteps,
                                   unsigned int maxSamples);
#endif
