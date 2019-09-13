// File Name : DEBUG_CONSTANTS.hpp
#ifndef DEBUG_CONSTANTS_H_
#define DEBUG_CONSTANTS_H_

#define TEST_SSR_FFT
#define NO_DATA_FRAMES_TO_SIMULATE 5

/// MAX_PERCENT_ERROR_IN_SAMPLE is the maximum allowed error in % when comparison when comparing golden and produced
/// sample, if sample differ by less than MAX_PERCENT_ERROR_IN_SAMPLE %
// it is not counted as error
#define MAX_PERCENT_ERROR_IN_SAMPLE 10

/// MAX_ALLOWED_PERCENTAGE_OF_SAMPLES_IN_ERROR is %tage of total errors allowed in any simulation(per frame) to pass if
/// the errors in any frame (number of mismatches) are larger than
// MAX_ALLOWED_PERCENTAGE_OF_SAMPLES_IN_ERROR % the simulation fails.
#define MAX_ALLOWED_PERCENTAGE_OF_SAMPLES_IN_ERROR 5

#define CHECK_COVEARAGE                                                                        \
    std::cout << "\n\n\n\nCovered;;;;;;;;\n" << __FILE__ << "Line:" << __LINE__ << "<<\n\n\n"; \
    exit(1)

#endif // !DEBUG_CONSTANTS_H
