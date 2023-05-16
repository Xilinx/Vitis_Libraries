/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

/**
 *
 * @file parse_date.hpp
 * @brief header file for date parser.
 *
 * @detail The input date format should be a string like one of the following,
 * 1) "YYYY-MM-DD HH:MM:SS"
 * 2) "YYYY-MM-DD HH:MM"
 * 3) "YYYY-MM-DD HH"
 * 4) "YYYY-MM-DD"
 * and the output is the number of seconds since 1970-01-01 00:00:00 UTC in integer.
 *
 */

#ifndef XF_DATA_ANALYTICS_L1_DATAFRAME_PARSE_DATE_HPP
#define XF_DATA_ANALYTICS_L1_DATAFRAME_PARSE_DATE_HPP

#include <ap_int.h>
#include <hls_stream.h>

#if !defined(__SYNTHESIS__) && __XF_DATA_ANALYTICS_L1_PARSE_DATE_DEBUG__ == 1
#include <iostream>
#include <stdlib.h>
#endif

namespace xf {
namespace data_analytics {
namespace dataframe {
namespace internal {

/**
 *
 * @brief Impletmentation to parse the input string to
 * the specific date with the required data type.
 *
 * The implementation is designed for better performance.
 *
 * @param inStrm Input string.
 * @param endInStrm End flag to signal the end of the input strings.
 * @param vldStrm Valid flag for each character of input string.
 * @param yearStrm Year of the date.
 * @param monthStrm Month of the date.
 * @param dayStrm Day of the date.
 * @param hourStrm Hour of the time.
 * @param minStrm Minute of the time.
 * @param secStrm Second of the time.
 * @param endOutStrm End flag to signal the end of the Date/Time.
 *
 */

static void str2Date(hls::stream<ap_uint<8> >& inStrm,
                     hls::stream<bool>& endInStrm,
                     hls::stream<bool>& vldStrm,
                     hls::stream<int>& yearStrm,
                     hls::stream<char>& monthStrm,
                     hls::stream<char>& dayStrm,
                     hls::stream<char>& hourStrm,
                     hls::stream<char>& minStrm,
                     hls::stream<char>& secStrm,
                     hls::stream<bool>& endOutStrm) {
    bool vld = true;
    int year = 0;
    char month = 0;
    char day = 0;
    char hour = 0;
    char min = 0;
    char sec = 0;
    ap_uint<3> currPtr = 0;

    bool end = false;
    bool nb_1 = false;
    bool nb_2 = false;
    bool nb_3 = false;
    while (!end) {
#pragma HLS pipeline II = 1
        if (!vld && nb_1 && nb_2 && nb_3) {
            // emit outputs
            yearStrm.write(year);
            monthStrm.write(month);
            dayStrm.write(day);
            hourStrm.write(hour);
            minStrm.write(min);
            secStrm.write(sec);
            endOutStrm.write(false);
            year = 0;
            month = 0;
            day = 0;
            hour = 0;
            min = 0;
            sec = 0;
            currPtr = 0;
        }

        ap_uint<8> in;
        nb_1 = vldStrm.read_nb(vld);
        nb_2 = inStrm.read_nb(in);
        nb_3 = endInStrm.read_nb(end);
        if (vld && nb_1 && nb_2 && nb_3) {
            // start or end, do nothing
            if (in == '"') {
                // hit the separator, move to next section
            } else if (in == '-' || in == ' ' || in == ':') {
                currPtr++;
                // string to corresponding integer
            } else {
                if (currPtr == 0) {
                    year = year * 10 + (in - '0');
                } else if (currPtr == 1) {
                    month = month * 10 + (in - '0');
                } else if (currPtr == 2) {
                    day = day * 10 + (in - '0');
                } else if (currPtr == 3) {
                    hour = hour * 10 + (in - '0');
                } else if (currPtr == 4) {
                    min = min * 10 + (in - '0');
                } else if (currPtr == 5) {
                    sec = sec * 10 + (in - '0');
                }
            }
        }
    }

    endOutStrm.write(true);

} // end str2Date

/**
 *
 * @brief Overloaded function for non-blocking read structure
 *
 * The implementation is designed for better performance.
 *
 * @param inStrm Input string and its corresponding valid flag <char, bool>.
 * @param endInStrm End flag to signal the end of the input strings.
 * @param vldStrm Valid flag for each character of input string.
 * @param outStrm Output date stream <year, month, day, hour, min, sec>.
 * @param endOutStrm End flag to signal the end of the out stream.
 *
 */

static void str2Date(hls::stream<ap_uint<9> >& inStrm,
                     hls::stream<bool>& endInStrm,
                     hls::stream<ap_uint<40> >& outStrm,
                     hls::stream<bool>& endOutStrm) {
    ap_uint<5> currChar = 0;
    ap_uint<40> out = 0;

    bool end = endInStrm.read();
    endOutStrm.write(false);
    while (!end) {
#pragma HLS pipeline II = 1
        bool endVld = endInStrm.read_nb(end);
        // avoid over-read on data port
        if (!end) {
            ap_uint<9> in;
            bool inVld = inStrm.read_nb(in);
            // uint8_t in_char = in.range(8,1);
            // printf("in %c\n", in_char);
            // data is successfully read
            if (inVld) {
                // data is valid
                if (in.range(0, 0)) {
                    // year
                    if (currChar > 0 && currChar < 5) {
#ifndef __SYNTHESIS__
                        if (!(in.range(8, 1) >= '0' && in.range(8, 1) <= '9')) {
                            std::cout << "Invalid number for year." << std::endl;
                            abort();
                        }
#endif
                        out.range(39, 26) = out.range(39, 26) * 10 + (in.range(8, 1) - '0');
#if !defined(__SYNTHESIS__) && __XF_DATA_ANALYTICS_L1_PARSE_DATE_DEBUG__ == 1
                        std::cout << "year.tmp = " << out.range(39, 26) << std::endl;
#endif
                        // separator between year and month
                    } else if (currChar == 5) {
#ifndef __SYNTHESIS__
                        if (in.range(8, 1) != '-') {
                            std::cout << "Invalid separator between year and month." << std::endl;
                            abort();
                        }
#endif
                        // month
                    } else if (currChar > 5 && currChar < 8) {
#ifndef __SYNTHESIS__
                        if (!(in.range(8, 1) >= '0' && in.range(8, 1) <= '9')) {
                            std::cout << "Invalid number for month." << std::endl;
                            abort();
                        }
#endif
                        out.range(25, 22) = out.range(25, 22) * 10 + (in.range(8, 1) - '0');
#if !defined(__SYNTHESIS__) && __XF_DATA_ANALYTICS_L1_PARSE_DATE_DEBUG__ == 1
                        std::cout << "month.tmp = " << out.range(25, 22) << std::endl;
#endif
                        // separator between month and day
                    } else if (currChar == 8) {
#ifndef __SYNTHESIS__
                        if (in.range(8, 1) != '-') {
                            std::cout << "Invalid separator between month and day." << std::endl;
                            abort();
                        }
#endif
                        // day
                    } else if (currChar > 8 && currChar < 11) {
#ifndef __SYNTHESIS__
                        if (!(in.range(8, 1) >= '0' && in.range(8, 1) <= '9')) {
                            std::cout << "Invalid number for day." << std::endl;
                            abort();
                        }
#endif
                        out.range(21, 17) = out.range(21, 17) * 10 + (in.range(8, 1) - '0');
#if !defined(__SYNTHESIS__) && __XF_DATA_ANALYTICS_L1_PARSE_DATE_DEBUG__ == 1
                        std::cout << "day.tmp = " << out.range(21, 17) << std::endl;
#endif
                        // separator between day and hour
                    } else if (currChar == 11) {
#ifndef __SYNTHESIS__
                        if (in.range(8, 1) != ' ' && in.range(8, 1) != '"') {
                            std::cout << "Invalid separator between day and hour." << std::endl;
                            abort();
                        }
#endif
                        // hour
                    } else if (currChar > 11 && currChar < 14) {
#ifndef __SYNTHESIS__
                        if (!(in.range(8, 1) >= '0' && in.range(8, 1) <= '9')) {
                            std::cout << "Invalid number for hour." << std::endl;
                            abort();
                        }
#endif
                        out.range(16, 12) = out.range(16, 12) * 10 + (in.range(8, 1) - '0');
#if !defined(__SYNTHESIS__) && __XF_DATA_ANALYTICS_L1_PARSE_DATE_DEBUG__ == 1
                        std::cout << "hour.tmp = " << out.range(16, 12) << std::endl;
#endif
                        // separator between hour and min
                    } else if (currChar == 14) {
#ifndef __SYNTHESIS__
                        if (in.range(8, 1) != ':' && in.range(8, 1) != '"') {
                            std::cout << "Invalid separator between hour and min." << std::endl;
                            abort();
                        }
#endif
                        // min
                    } else if (currChar > 14 && currChar < 17) {
#ifndef __SYNTHESIS__
                        if (!(in.range(8, 1) >= '0' && in.range(8, 1) <= '9')) {
                            std::cout << "Invalid number for min." << std::endl;
                            abort();
                        }
#endif
                        out.range(11, 6) = out.range(11, 6) * 10 + (in.range(8, 1) - '0');
#if !defined(__SYNTHESIS__) && __XF_DATA_ANALYTICS_L1_PARSE_DATE_DEBUG__ == 1
                        std::cout << "min.tmp = " << out.range(11, 6) << std::endl;
#endif
                        // separator between min and sec
                    } else if (currChar == 17) {
#ifndef __SYNTHESIS__
                        if (in.range(8, 1) != ':' && in.range(8, 1) != '"') {
                            std::cout << "Invalid separator between min and sec." << std::endl;
                            abort();
                        }
#endif
                        // sec
                    } else if (currChar > 17 && currChar < 20) {
#ifndef __SYNTHESIS__
                        if (!(in.range(8, 1) >= '0' && in.range(8, 1) <= '9')) {
                            std::cout << "Invalid number for sec." << std::endl;
                            abort();
                        }
#endif
                        out.range(5, 0) = out.range(5, 0) * 10 + (in.range(8, 1) - '0');
#if !defined(__SYNTHESIS__) && __XF_DATA_ANALYTICS_L1_PARSE_DATE_DEBUG__ == 1
                        std::cout << "sec.tmp = " << out.range(5, 0) << std::endl;
#endif
                    }
                    // move to next char
                    currChar++;
                    // last char of the string
                } else {
#ifndef __SYNTHESIS__
                    if (in.range(8, 1) == 'n') {
                    } else if (currChar < 11) {
                        std::cout << "Invalid time point. At least year, month, and day should be given." << std::endl;
                        abort();
                    } else if (currChar == 13) {
                        std::cout << "Invalid time point.The hour must be 2-digit" << std::endl;
                        abort();
                    } else if (currChar == 16) {
                        std::cout << "Invalid time point.The minute must be 2-digit" << std::endl;
                        abort();
                    } else if (currChar == 19) {
                        std::cout << "Invalid time point.The second must be 2-digit" << std::endl;
                        abort();
                    } else if (currChar > 21) {
                        std::cout << "Invalid time point. Input is too long for a complete time point." << std::endl;
                        abort();
                    }
#endif
#if !defined(__SYNTHESIS__) && __XF_DATA_ANALYTICS_L1_PARSE_DATE_DEBUG__ == 1
                    std::cout << "Date:" << std::endl;
                    std::cout << "year = " << out.range(39, 26) << std::endl;
                    std::cout << "month = " << out.range(25, 22) << std::endl;
                    std::cout << "day = " << out.range(21, 17) << std::endl;
                    std::cout << "hour = " << out.range(16, 12) << std::endl;
                    std::cout << "min = " << out.range(11, 6) << std::endl;
                    std::cout << "sec = " << out.range(5, 0) << std::endl;
                    std::cout << std::endl;
#endif
                    outStrm.write(out);
                    endOutStrm.write(false);
                    out = 0;
                    currChar = 0;
                }
            }
        }
    }

    endOutStrm.write(true);

#if !defined(__SYNTHESIS__) && __XF_DATA_ANALYTICS_L1_PARSE_DATE_DEBUG__ == 1
    std::cout << "out.size = " << outStrm.size() << std::endl;
    std::cout << "endOut.size = " << endOutStrm.size() << std::endl;
#endif

} // end str2Date

/**
 *
 * @brief Impletmentation to parse a specific date to
 * the number of seconds since 1970-01-01 00:00:00 UTC.
 *
 * The implementation is designed for better performance.
 *
 * @param yearStrm Year of the date.
 * @param monthStrm Month of the date.
 * @param dayStrm Day of the date.
 * @param hourStrm Hour of the time.
 * @param minStrm Minute of the time.
 * @param secStrm Second of the time.
 * @param endInStrm End flag to signal the end of the input stream.
 * @param numSecStrm Number of seconds since 1970-01-01 00:00:00.
 *
 */

static void convertTime(hls::stream<int>& yearStrm,
                        hls::stream<char>& monthStrm,
                        hls::stream<char>& dayStrm,
                        hls::stream<char>& hourStrm,
                        hls::stream<char>& minStrm,
                        hls::stream<char>& secStrm,
                        hls::stream<bool>& endInStrm,
                        hls::stream<ap_uint<64> >& numSecStrm) {
    bool not_empty = true;
    bool end = endInStrm.read();
    while (!end) {
#pragma HLS pipeline II = 1
        // read in date & time
        if (not_empty) {
            int year;
            char month, day, hour, min, sec;
            bool nb_0, nb_1, nb_2, nb_3, nb_4, nb_5;
            nb_0 = yearStrm.read_nb(year);
            nb_1 = monthStrm.read_nb(month);
            nb_2 = dayStrm.read_nb(day);
            nb_3 = hourStrm.read_nb(hour);
            nb_4 = minStrm.read_nb(min);
            nb_5 = secStrm.read_nb(sec);
            if (nb_0 && nb_1 && nb_2 && nb_3 && nb_4 && nb_5) {
                int64_t y, m, d, ysub, era, yoe, doy, doe, days, numSec;
                // core of the algorithm
                y = year - (month <= 2);
                m = static_cast<unsigned>(month);
                d = static_cast<unsigned>(day);
                if (y < 0) {
                    ysub = y - 399;
                } else {
                    ysub = y;
                }
                era = ysub / 400;
                yoe = static_cast<unsigned>(y - era * 400);
                doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
                doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
                days = era * 146097 + doe - 719468;
                numSec = days * 24 * 60 * 60 + hour * 60 * 60 + min * 60 + sec;

                // emit results
                ap_uint<64> out = numSec;
                numSecStrm.write(numSec);
            }
        }

        not_empty = endInStrm.read_nb(end);
    }

} // end convertTime

/**
 *
 * @brief Impletmentation to parse a specific date to
 * the number of seconds since 1970-01-01 00:00:00 UTC.
 *
 * The implementation is designed for better performance.
 *
 * @param inStrm Input date stream <year, month, day, hour, min, sec>.
 * @param endInStrm End flag to signal the end of the input stream.
 * @param numSecStrm Number of seconds since 1970-01-01 00:00:00.
 *
 */

static void convertTime(hls::stream<ap_uint<40> >& inStrm,
                        hls::stream<bool>& endInStrm,
                        hls::stream<ap_uint<64> >& numSecStrm) {
    bool end = endInStrm.read();
    while (!end) {
#pragma HLS pipeline II = 1
        // use skipEnd to prevent e-stream being over-read
        bool endVld = endInStrm.read_nb(end);
        // avoid over-read on data port
        if (!end) {
            ap_uint<40> in;
            bool inVld = inStrm.read_nb(in);
            // data is successfully read
            if (inVld) {
                // save the date & time accordingly
                int year = static_cast<int>(in.range(39, 26));
                char month = static_cast<char>(in.range(25, 22));
                char day = static_cast<char>(in.range(21, 17));
                char hour = static_cast<char>(in.range(16, 12));
                char min = static_cast<char>(in.range(11, 6));
                char sec = static_cast<char>(in.range(5, 0));

                // intermediate regs for calculating the number of seconds
                int64_t y, m, d, ysub, era, yoe, doy, doe, days, numSec;

                // core of the algorithm
                y = year - (month <= 2);
                m = static_cast<unsigned>(month);
                d = static_cast<unsigned>(day);
                if (y < 0) {
                    ysub = y - 399;
                } else {
                    ysub = y;
                }
                era = ysub / 400;
                yoe = static_cast<unsigned>(y - era * 400);
                doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
                doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
                days = era * 146097 + doe - 719468;
                numSec = days * 24 * 60 * 60 + hour * 60 * 60 + min * 60 + sec;

                // emit results
                ap_uint<64> out = numSec;
                numSecStrm.write(out);
            }
        }
    }

} // end convertTime

/**
 *
 * @brief Top of date parser.
 *
 * The implementation is designed for better performance.
 *
 * @param inStrm Input string.
 * @param endInStrm End flag to signal the end of the input string.
 * @param vldStrm Valid flag for each character of input string.
 * @param numSecStrm Number of seconds since 1970-01-01 00:00:00.
 *
 */

static void parseDate(hls::stream<ap_uint<8> >& inStrm,
                      hls::stream<bool>& endInStrm,
                      hls::stream<bool>& vldStrm,
                      hls::stream<ap_uint<64> >& numSecStrm) {
#pragma HLS dataflow

    hls::stream<int> yearStrm;
#pragma HLS stream variable = yearStrm depth = 4
#pragma HLS bind_storage variable = yearStrm type = FIFO impl = LUTRAM
    hls::stream<char> monthStrm;
#pragma HLS stream variable = monthStrm depth = 4
#pragma HLS bind_storage variable = monthStrm type = FIFO impl = LUTRAM
    hls::stream<char> dayStrm;
#pragma HLS stream variable = dayStrm depth = 4
#pragma HLS bind_storage variable = dayStrm type = FIFO impl = LUTRAM
    hls::stream<char> hourStrm;
#pragma HLS stream variable = hourStrm depth = 4
#pragma HLS bind_storage variable = hourStrm type = FIFO impl = LUTRAM
    hls::stream<char> minStrm;
#pragma HLS stream variable = minStrm depth = 4
#pragma HLS bind_storage variable = minStrm type = FIFO impl = LUTRAM
    hls::stream<char> secStrm;
#pragma HLS stream variable = secStrm depth = 4
#pragma HLS bind_storage variable = secStrm type = FIFO impl = LUTRAM
    hls::stream<bool> endDateStrm;
#pragma HLS stream variable = endDateStrm depth = 4
#pragma HLS bind_storage variable = endDateStrm type = FIFO impl = LUTRAM

    str2Date(inStrm, endInStrm, vldStrm, yearStrm, monthStrm, dayStrm, hourStrm, minStrm, secStrm, endDateStrm);

    convertTime(yearStrm, monthStrm, dayStrm, hourStrm, minStrm, secStrm, endDateStrm, numSecStrm);

} // end parseDate

/**
 *
 * @brief Top of date parser (overloaded function with non-blocking read sturcture).
 *
 * The implementation is modified for better performance.
 *
 * @param inStrm Input string and its corresponding valid flag <char, bool>.
 * @param endInStrm End flag to signal the end of the input string.
 * @param numSecStrm Number of seconds since 1970-01-01 00:00:00.
 *
 */

static void parseDate(hls::stream<ap_uint<9> >& inStrm,
                      hls::stream<bool>& endInStrm,
                      hls::stream<ap_uint<64> >& numSecStrm) {
#pragma HLS dataflow

    hls::stream<ap_uint<40> > interStrm;
#pragma HLS stream variable = interStrm depth = 2
#pragma HLS bind_storage variable = interStrm type = FIFO impl = LUTRAM
    hls::stream<bool> endInterStrm;
#pragma HLS stream variable = endInterStrm depth = 2
#pragma HLS bind_storage variable = endInterStrm type = FIFO impl = LUTRAM

    str2Date(inStrm, endInStrm, interStrm, endInterStrm);

    convertTime(interStrm, endInterStrm, numSecStrm);

} // end parseDate
} // namespace internal
} // dataframe
} // namespace data_analytics
} // namespace xf

#endif
