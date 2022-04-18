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

#include "defs.hpp"
#include "xilinxlouvain.hpp"
#include "xcl2.hpp"

using namespace std;
// WARNING: This will overwrite the original graphNew data structure to
//         minimize memory footprint
// Return: C_orig will hold the cluster ids for vertices in the original graphNew
//         Assume C_orig is initialized appropriately
// WARNING: Graph G will be destroyed at the end of this routine

void inline PhaseLoop_UpdatingC_org(int phase, long NV, long NV_G, long* C, long* C_orig) {
    if (phase == 1) {
#pragma omp parallel for
        for (long i = 0; i < NV; i++) {
            C_orig[i] = C[i]; // After the first phase
        }
    } else {
#pragma omp parallel for
        for (long i = 0; i < NV; i++) {
            assert(C_orig[i] < NV_G);
            if (C_orig[i] >= 0) C_orig[i] = C[C_orig[i]]; // Each cluster in a previous phase becomes a vertex
        }
    }
    printf("Done updating C_orig\n");
}

/*
double PhaseLoop_UsingFPGA_Prep_Init_buff_host(int numColors,
                                               graphNew* G,
                                               long* M,
                                               double opts_C_thresh,
                                               double* currMod,
                                               // Updated variables
                                               int* colors,
                                               KMemorys_host* buff_host) {
    int edgeNum;
    double time1 = omp_get_wtime();
    assert(numColors < COLORS);
    long vertexNum = G->numVertices;
    long* vtxPtr = G->edgeListPtrs;
    edge* vtxInd = G->edgeList;
    long NE = G->numEdges;
    long NEx2 = NE << 1;
    long NE1 = NEx2 < (1 << 26) ? NEx2 : (1 << 26); // 256MB/sizeof(int/float)=64M

    long cnt_e = 0;
    for (int i = 0; i < vertexNum + 1; i++) {
        buff_host[0].offsets[i] = (int)vtxPtr[i];
        if (i != vertexNum) {
            if (M[i] < 0) buff_host[0].offsets[i] = (int)(0x80000000 | (unsigned int)vtxPtr[i]);
        } else
            buff_host[0].offsets[i] = (int)(vtxPtr[i]);
    }
    edgeNum = buff_host[0].offsets[vertexNum];
    for (int i = 0; i < vertexNum; i++) {
        int adj1 = vtxPtr[i];
        int adj2 = vtxPtr[i + 1];
        for (int j = adj1; j < adj2; j++) {
            if (cnt_e < NE1) {
                buff_host[0].indices[j] = (int)vtxInd[j].tail;
                buff_host[0].weights[j] = vtxInd[j].weight;
            } else {
                buff_host[0].indices2[j - NE1] = (int)vtxInd[j].tail;
                buff_host[0].weights2[j - NE1] = vtxInd[j].weight;
            }
            cnt_e++;
        }
    }
    for (int i = 0; i < vertexNum; i++) {
        buff_host[0].colorAxi[i] = colors[i];
    }
    buff_host[0].config0[0] = vertexNum;
    buff_host[0].config0[1] = numColors;
    buff_host[0].config0[2] = 0;
    buff_host[0].config0[3] = edgeNum;
    buff_host[0].config1[0] = opts_C_thresh;
    buff_host[0].config1[1] = currMod[0];
    time1 = omp_get_wtime() - time1;
    return time1;
}
*/
// double PhaseLoop_UsingFPGA_Prep_Read_buff_host(long vertexNum,
//                                                KMemorys_host* buff_host,
//                                                int* eachItrs,
//                                                // output
//                                                long* C,
//                                                int* eachItr,
//                                                double* currMod) {
//     double time1 = omp_get_wtime();
//     // updating
//     eachItrs[0] = buff_host[0].config0[2];
//     eachItr[0] = buff_host[0].config0[2];
//     currMod[0] = buff_host[0].config1[1];
//     for (int i = 0; i < vertexNum; i++) {
//         C[i] = (long)buff_host[0].cidPrev[i];
//     }
//     time1 = omp_get_wtime() - time1;
//     return time1;
// }
/*
long* CreateM(long NV_new, long NV_orig, long* C_orig, long* M_orig) {
    long* M = (long*)malloc(NV_new * sizeof(long));
    assert(M);
    memset(M, 0, NV_new * sizeof(long));
    for (int i = 0; i < NV_orig; i++) {
        if (M_orig[i] < 0) M[C_orig[i]] = M_orig[i];
    }
    return M;
}

double PhaseLoop_CommPostProcessing_par(GLV* pglv_orig,
                                        GLV* pglv_iter,
                                        int numThreads,
                                        double opts_threshold,
                                        bool opts_coloring,
                                        // modified:
                                        bool& nonColor,
                                        int& phase,
                                        int& totItr,
                                        long& numClusters,
                                        double& totTimeBuildingPhase,
                                        double& time_renum,
                                        double& time_C,
                                        double& time_M,
                                        double& time_buid,
                                        double& time_set) {
    double time1 = 0;
    time1 = omp_get_wtime();
    time_renum = time1;
    graphNew* Gnew;
    numClusters = renumberClustersContiguously_ghost(pglv_iter->C, pglv_iter->G->numVertices, pglv_iter->NVl);
    printf("Number of unique clusters: %ld\n", numClusters);
    time_renum = omp_get_wtime() - time_renum;

    time_C = omp_get_wtime();
    PhaseLoop_UpdatingC_org(phase, pglv_orig->NV, pglv_iter->NV, pglv_iter->C, pglv_orig->C);
    time_C = omp_get_wtime() - time_C;

    time_M = omp_get_wtime();
    long* M_new = CreateM(numClusters, pglv_orig->NV, pglv_orig->C, pglv_orig->M);
    time_M = omp_get_wtime() - time_M;

    Gnew = (graphNew*)malloc(sizeof(graphNew));
    assert(Gnew != 0);
    double tmpTime = buildNextLevelGraphOpt(pglv_iter->G, Gnew, pglv_iter->C, numClusters, numThreads);
    totTimeBuildingPhase += tmpTime;
    time_buid = tmpTime;

    time_set = omp_get_wtime();
    pglv_iter->SetByOhterG(Gnew, M_new);
    time_set = omp_get_wtime() - time_set;

    time1 = omp_get_wtime() - time1;

    return time1;
}
*/

/*
double PhaseLoop_UsingFPGA_Prep_Init_buff_host_prune(//DAIS
                int 				numColors,
                graphNew*        	G,
                long*         		M,
                double        		opts_C_thresh,
                double*        		currMod,
                //Updated variables
            int*          		colors,
                KMemorys_host_prune *buff_host)
{

        int edgeNum;
        double time1 = omp_get_wtime();
        assert(numColors < COLORS);
        long vertexNum = G->numVertices;
        long* vtxPtr   = G->edgeListPtrs;
        edge* vtxInd   = G->edgeList;
        long NE = G->numEdges;
        long NEx2 = NE<<1;
        long NE1 = NEx2< (1<<26)? NEx2 : (1<<26);//256MB/sizeof(int/float)=64M

        long cnt_e=0;
            for (int i = 0; i < vertexNum + 1; i++) {
                buff_host[0].offsets[i] = (int)vtxPtr[i];
                if(i!=vertexNum){
                        if(M[i]<0)
                                buff_host[0].offsets[i]  = (int) (0x80000000 |(unsigned int)vtxPtr[i]);
                }else
                    buff_host[0].offsets[i]  = (int) ( vtxPtr[i]);
            }
            edgeNum = buff_host[0].offsets[vertexNum];
            for (int i = 0; i < vertexNum + 1; i++) {
                buff_host[0].offsets[i] = (int)vtxPtr[i];
                buff_host[0].offsetsdup[i] = buff_host[0].offsets[i];//
                if(i!=vertexNum){
                        if(M[i]<0)
                                buff_host[0].offsets[i]  = (int) (0x80000000 |(unsigned int)vtxPtr[i]);
                }else
                    buff_host[0].offsets[i]  = (int) ( vtxPtr[i]);
            }
            edgeNum = buff_host[0].offsets[vertexNum];
            std::cout << "INFO: for test  1" << std::endl;
            for (int i = 0; i < vertexNum; i++) {
                int adj1 = vtxPtr[i];
                int adj2 = vtxPtr[i + 1];
                buff_host[0].flag[i] = 0;//
                buff_host[0].flagUpdate[i] = 0;//
                for (int j = adj1; j < adj2; j++) {
                        if(cnt_e<NE1){
                                buff_host[0].indices[j] = (int)vtxInd[j].tail;
                                buff_host[0].indicesdup[j] = (int)vtxInd[j].tail;
                                                buff_host[0].weights[j] = vtxInd[j].weight;
                        }else{
                                buff_host[0].indices2[j-NE1] = (int)vtxInd[j].tail;
                                buff_host[0].indicesdup2[j-NE1] = (int)vtxInd[j].tail;
                                                buff_host[0].weights2[j-NE1] = vtxInd[j].weight;
                        }
                    cnt_e++;
                }
            }
            for (int i = 0; i < vertexNum; i++) {
                buff_host[0].colorAxi[i] = colors[i];
            }
            std::cout << "INFO: for test  2" << std::endl;
            buff_host[0].config0[0] = vertexNum;
            buff_host[0].config0[1] = numColors;
            buff_host[0].config0[2] = 0;
            buff_host[0].config0[3] = edgeNum;
            buff_host[0].config0[4] = 0;// totItr? 0?

            buff_host[0].config1[0] = opts_C_thresh;
            buff_host[0].config1[1] = currMod[0];
            time1  = omp_get_wtime() - time1;
            return time1;

            std::cout << "INFO: eachItrs" <<buff_host[0].config0[2] <<", "<<"eachItr[0] = "<<buff_host[0].config0[2]<<",
"<<"currMod[0] = "<<buff_host[0].config1[1]<< std::endl;
}

*/
/*
double PhaseLoop_UsingFPGA_Prep_Read_buff_host_prune(//DAIS
                long           		vertexNum,
                KMemorys_host_prune *buff_host,
                int            		*eachItrs,
                //output
                long*          		C,
                int            		*eachItr,
                double         		*currMod
                )
{
        double time1 = omp_get_wtime();
        //updating
        eachItrs[0] = buff_host[0].config0[2];
        eachItr[0] = buff_host[0].config0[2];
    currMod[0] = buff_host[0].config1[1];
    for (int i = 0; i < vertexNum; i++) {
        C[i] = (long)buff_host[0].cidPrev[i];
    }
    std::cout << " read INFO: eachItrs" <<eachItrs[0] <<", "<<"eachItr[0] = "<<eachItr[0] <<", "<<"currMod[0] =
"<<currMod[0]<< std::endl;
    time1  = omp_get_wtime() - time1;
    return time1;
}
*/
