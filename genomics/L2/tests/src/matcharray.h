/*
 * (c) Copyright 2022 Xilinx, Inc. All rights reserved.
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
 *
 */
#ifndef __MATCH_ARRAY__
#define __MATCH_ARRAY__

#include <string>

using namespace std;

class MatchArray {
   public:
    MatchArray(int numSamples, int readSz, int refSz);

    ~MatchArray();
    void populateArray(unsigned int* pairs, unsigned int* maxval);

   private:
    int m_counter;

    char** readStrings;
    char** refStrings;
    short* maxv;
    short* maxc;
    short* maxr;
    int numSamples;
    int readSize;
    int refSize;
};
#endif
