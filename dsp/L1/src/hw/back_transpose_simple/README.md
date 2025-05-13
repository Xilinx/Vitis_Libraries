This transpose performs the following operation. For ex: SSR = 4, POINT SIZE = 64

Cycle ------->
Input stream 0: 0 4 8 12 16 20 24 28 32 ... 60
Input stream 1: 1 5 9 13 17 21 25 29 33 ... 61
Input stream 2: 2 6 10 14 18 22 26 30 34 ... 62
Input stream 3: 3 7 11 15 19 23 27 31 35 ... 63

Output stream 0: 0 16 32 48 1 17 ... 51
Output stream 1: 4 20 36 52 5 21 ... 55
Output stream 2: 8 24 40 56 9 25 ... 59
Output stream 3: 12 28 44 60 13 29 ... 63

i.e, Upon interleaving the output data from all the streams, you get the all the input data from input stream 0, followed by the input data from input stream 1 and so on.

## License

 Copyright (C) 2019-2022, Xilinx, Inc.
 Copyright (C) 2022-2025, Advanced Micro Devices, Inc.

Terms and Conditions <https://www.amd.com/en/corporate/copyright>