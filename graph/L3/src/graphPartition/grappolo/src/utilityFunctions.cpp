// ***********************************************************************
//
//            Grappolo: A C++ library for graph clustering
//               Mahantesh Halappanavar (hala@pnnl.gov)
//               Pacific Northwest National Laboratory
//
// ***********************************************************************
//
//       Copyright (2014) Battelle Memorial Institute
//                      All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// ************************************************************************

#include "RngStream.hpp"
#include "defs.hpp"

using namespace std;

void generateRandomNumbers(double* RandVec, long size) {
    int nT;
#pragma omp parallel
    { nT = omp_get_num_threads(); }
#ifdef PRINT_DETAILED_STATS_
    printf("Within generateRandomNumbers() -- Number of threads: %d\n", nT);
#endif
    // Initialize parallel pseudo-random number generator
    unsigned long seed[6] = {1, 2, 3, 4, 5, 6};
    RngStream::SetPackageSeed(seed);
    RngStream RngArray[nT]; // array of RngStream Objects

    long block = size / nT;
#ifdef PRINT_DETAILED_STATS_
    cout << "Each thread will add " << block << " edges\n";
#endif
// Each thread will generate m/nT edges each
// double start = omp_get_wtime();// unused variable ‘start’
#pragma omp parallel
    {
        int myRank = omp_get_thread_num();
#pragma omp for schedule(static)
        for (long i = 0; i < size; i++) {
            RandVec[i] = RngArray[myRank].RandU01();
        }
    } // End of parallel region
} // End of generateRandomNumbers()

void displayGraphCharacteristics(graphNew* G) {
    printf("Within displayGraphCharacteristics()\n");
    long sum = 0, sum_sq = 0;
    double average, avg_sq, variance, std_dev;
    long maxDegree = 0;
    long isolated = 0;
    long degreeOne = 0;
    long NS = G->sVertices;
    long NV = G->numVertices;
    long NT = NV - NS;
    long NE = G->numEdges;
    long* vtxPtr = G->edgeListPtrs;
    long tNV = NV; // Number of vertices

    if ((NS == 0) || (NS == NV)) { // Nonbiparite graphNew
        for (long i = 0; i < NV; i++) {
            long degree = vtxPtr[i + 1] - vtxPtr[i];
            sum_sq += degree * degree;
            sum += degree;
            if (degree > maxDegree) maxDegree = degree;
            if (degree == 0) isolated++;
            if (degree == 1) degreeOne++;
        }
        average = (double)sum / tNV;
        avg_sq = (double)sum_sq / tNV;
        variance = avg_sq - (average * average);
        std_dev = sqrt(variance);

        printf("*******************************************\n");
        printf("General Graph: Characteristics :\n");
        printf("*******************************************\n");
        printf("Number of vertices   :  %ld\n", NV);
        printf("Number of edges      :  %ld\n", NE);
        printf("Maximum out-degree is:  %ld\n", maxDegree);
        printf("Average out-degree is:  %lf\n", average);
        printf("Expected value of X^2:  %lf\n", avg_sq);
        printf("Variance is          :  %lf\n", variance);
        printf("Standard deviation   :  %lf\n", std_dev);
        printf("Isolated vertices    :  %ld (%3.2lf%%)\n", isolated, ((double)isolated / tNV) * 100);
        printf("Degree-one vertices  :  %ld (%3.2lf%%)\n", degreeOne, ((double)degreeOne / tNV) * 100);
        printf("Density              :  %lf%%\n", ((double)NE / (NV * NV)) * 100);
        printf("*******************************************\n");

    }      // End of nonbipartite graphNew
    else { // Bipartite graphNew

        // Compute characterisitcs from S side:
        for (long i = 0; i < NS; i++) {
            long degree = vtxPtr[i + 1] - vtxPtr[i];
            sum_sq += degree * degree;
            sum += degree;
            if (degree > maxDegree) maxDegree = degree;
            if (degree == 0) isolated++;
            if (degree == 1) degreeOne++;
        }
        average = (double)sum / NS;
        avg_sq = (double)sum_sq / NS;
        variance = avg_sq - (average * average);
        std_dev = sqrt(variance);

        printf("*******************************************\n");
        printf("Bipartite Graph: Characteristics of S:\n");
        printf("*******************************************\n");
        printf("Number of S vertices :  %ld\n", NS);
        printf("Number of T vertices :  %ld\n", NT);
        printf("Number of edges      :  %ld\n", NE);
        printf("Maximum out-degree is:  %ld\n", maxDegree);
        printf("Average out-degree is:  %lf\n", average);
        printf("Expected value of X^2:  %lf\n", avg_sq);
        printf("Variance is          :  %lf\n", variance);
        printf("Standard deviation   :  %lf\n", std_dev);
        printf("Isolated (S)vertices :  %ld (%3.2lf%%)\n", isolated, ((double)isolated / NS) * 100);
        printf("Degree-one vertices  :  %ld (%3.2lf%%)\n", degreeOne, ((double)degreeOne / tNV) * 100);
        printf("Density              :  %lf%%\n", ((double)NE / (NS * NS)) * 100);
        printf("*******************************************\n");

        sum = 0;
        sum_sq = 0;
        maxDegree = 0;
        isolated = 0;
        // Compute characterisitcs from T side:
        for (long i = NS; i < NV; i++) {
            long degree = vtxPtr[i + 1] - vtxPtr[i];
            sum_sq += degree * degree;
            sum += degree;
            if (degree > maxDegree) maxDegree = degree;
            if (degree == 0) isolated++;
            if (degree == 1) degreeOne++;
        }

        average = (double)sum / NT;
        avg_sq = (double)sum_sq / NT;
        variance = avg_sq - (average * average);
        std_dev = sqrt(variance);

        printf("Bipartite Graph: Characteristics of T:\n");
        printf("*******************************************\n");
        printf("Number of T vertices :  %ld\n", NT);
        printf("Number of S vertices :  %ld\n", NS);
        printf("Number of edges      :  %ld\n", NE);
        printf("Maximum out-degree is:  %ld\n", maxDegree);
        printf("Average out-degree is:  %lf\n", average);
        printf("Expected value of X^2:  %lf\n", avg_sq);
        printf("Variance is          :  %lf\n", variance);
        printf("Standard deviation   :  %lf\n", std_dev);
        printf("Isolated (T)vertices :  %ld (%3.2lf%%)\n", isolated, ((double)isolated / NT) * 100);
        printf("Degree-one vertices  :  %ld (%3.2lf%%)\n", degreeOne, ((double)degreeOne / tNV) * 100);
        printf("Density              :  %lf%%\n", ((double)NE / (NT * NT)) * 100);
        printf("*******************************************\n");
    } // End of bipartite graphNew
}
