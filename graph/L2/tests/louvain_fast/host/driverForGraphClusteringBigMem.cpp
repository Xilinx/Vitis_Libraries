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

#include "defs.h"

using namespace std;

int main(int argc, char** argv) {
 
  //Parse Input parameters:
  clustering_parameters opts;
  if (!opts.parse(argc, argv)) {
    return -1;
  }
  int nT = 1; //Default is one thread
#pragma omp parallel
  {
      nT = omp_get_num_threads();
  }
  if (nT <= 1) {
	printf("The number of threads should be greater than one.\n");
	return 0;
  }
  double time1, time2;
  graphNew* G = (graphNew *) malloc (sizeof(graphNew));

  /* Step 2: Parse the graphNew in Matrix Market format */  
  int fType = opts.ftype; //File type
  char *inFile = (char*) opts.inFile;
  if(fType == 1)
     parse_MatrixMarket_Sym_AsGraph(G, inFile);
  else if(fType == 2)
     parse_Dimacs9FormatDirectedNewD(G, inFile);
  else if(fType == 3)
     parse_PajekFormat(G, inFile);
  else if(fType == 4)
     parse_PajekFormatUndirected(G, inFile);
  else if(fType == 5)
     loadMetisFileFormat(G, inFile); 
  else if(fType == 6)
     parse_DoulbedEdgeList(G, inFile); 
  else 
     parse_EdgeListBinary(G, inFile);
    
  displayGraphCharacteristics(G);
  int coloring = 0;
  if( opts.coloring )
     coloring = 1;

  /* Vertex Following option */
  if( opts.VF ) {
    	printf("Vertex following is enabled.\n");
        time1 = omp_get_wtime();
        long numVtxToFix = 0; //Default zero
        long *C = (long *) malloc (G->numVertices * sizeof(long)); assert(C != 0);
	numVtxToFix = vertexFollowing(G,C); //Find vertices that follow other vertices
	if( numVtxToFix > 0) {  //Need to fix things: build a new graphNew		      
	        printf("Graph will be modified -- %ld vertices need to be fixed.\n", numVtxToFix);          
		graphNew *Gnew = (graphNew *) malloc (sizeof(graphNew));
		long numClusters = renumberClustersContiguously(C, G->numVertices);		         
		buildNewGraphVF(G, Gnew, C, numClusters);
  		//Get rid of the old graphNew and store the new graphNew
		free(G->edgeListPtrs);
		free(G->edgeList);
		free(G);
		G = Gnew;		
	}
	free(C); //Free up memory
	printf("Graph after modifications:\n");
	displayGraphCharacteristics(G);
   }//End of if( VF == 1 )

   //Datastructures to store clustering information
   long NV = G->numVertices;
   long *C_orig = (long *) malloc (NV * sizeof(long)); assert(C_orig != 0);

   //Call the clustering algorithm: 
   if ( opts.strongScaling ) { //Strong scaling enabled
     //Retain the original copy of the graphNew:
     graphNew* G_original = (graphNew *) malloc (sizeof(graphNew)); //The original version of the graphNew
     time1 = omp_get_wtime();
     duplicateGivenGraph(G, G_original);
     time2 = omp_get_wtime();	
     printf("Time to duplicate : %lf\n", time2-time1);	
     
     //Run the algorithm in powers of two for the maximum number of threads available
     int tStack[] = {2,5,10,20,30,40,50,60,70,80};
     int nStack = 10, curThread=2;
     for(int iT = 0; iT < nStack; iT++) {
       curThread = tStack[iT];
       printf("\n\n***************************************\n");
       printf("Starting run with %d threads.\n", curThread);
       printf("***************************************\n");	
       //Call the clustering algorithm:
#pragma omp parallel for
       for (long i=0; i<G->numVertices; i++) {
	 C_orig[i] = -1;
       }
       runMultiPhaseLouvainAlgorithm(G, C_orig, coloring, opts.minGraphSize, opts.threshold, opts.C_thresh, curThread);
       //Increment thread and revert back to original graphNew
       if (curThread < tStack[nStack-1]) {
	 //Skip copying at the very end	
	 //Old graphNew is already destroyed in the above function
	 G = (graphNew *) malloc (sizeof(graphNew)); //Allocate new space
	 duplicateGivenGraph(G_original, G); //Copy the original graphNew to G		
       }
       curThread = curThread*2; //Increment by powers of two
     }//End of while()
   } else { //No strong scaling -- run once with max threads
#pragma omp parallel for
     for (long i=0; i<NV; i++) {
       C_orig[i] = -1;
     }
     runMultiPhaseLouvainAlgorithm(G, C_orig, coloring, opts.minGraphSize, opts.threshold, opts.C_thresh, nT); 
   }

   //Check if cluster ids need to be written to a file:
   if( opts.output ) {
       char outFile[256];
       sprintf(outFile,"%s_clustInfo", opts.inFile);
       printf("Cluster information will be stored in file: %s\n", outFile);
       FILE* out = fopen(outFile,"w");
       for(long i = 0; i<NV;i++) {
          fprintf(out,"%ld\n",C_orig[i]);
       }		
       fclose(out);
  }

  //Cleanup:
  if(C_orig != 0) free(C_orig);
  //Do not free G here -- it will be done in another routine.
  
  return 0;
}//End of main()
